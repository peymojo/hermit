//
//	Hermit
//	Copyright (C) 2017 Paul Young (aka peymojo)
//
//	This program is free software: you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation, either version 3 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <iostream>
#include <stack>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/XML/ParseXMLData.h"
#include "S3GetListObjectsXML.h"
#include "S3ListObjects.h"

namespace hermit {
	namespace s3 {
		namespace S3ListObjects_Impl {

			//
			class ProcessS3ListObjectsXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kListBucketResult,
					kIsTruncated,
					kContents,
					kKey,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessS3ListObjectsXMLClass(const HermitPtr& h_, ObjectKeyReceiverPtr receiver) :
				mH_(h_),
				mReceiver(receiver),
				mParseState(ParseState::kNew),
				mSawListBucketResult(false) {
				}
								
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				std::string GetIsTruncated() const {
					return mIsTruncated;
				}
				
				//
				std::string GetLastKey() const {
					return mLastKey;
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "ListBucketResult") {
							mSawListBucketResult = true;
							PushState(ParseState::kListBucketResult);
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kListBucketResult) {
						if (inStartTag == "Contents") {
							PushState(ParseState::kContents);
						}
						else if (inStartTag == "IsTruncated") {
							PushState(ParseState::kIsTruncated);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kContents) {
						if (inStartTag == "Key") {
							PushState(ParseState::kKey);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else {
						PushState(ParseState::kIgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == ParseState::kKey) {
						std::string key = inContent;
						if (mLastKey.empty() || (key > mLastKey)) {
							mLastKey = key;
						}
						
						if (!mReceiver->OnOneKey(mH_, inContent)) {
							return xml::kParseXMLStatus_Cancel;
						}
					}
					else if (mParseState == ParseState::kIsTruncated) {
						mIsTruncated = inContent;
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					PopState();
					return xml::kParseXMLStatus_OK;
				}
				
				//
				void PushState(ParseState inNewState) {
					mParseStateStack.push(mParseState);
					mParseState = inNewState;
				}
				
				//
				void PopState() {
					mParseState = mParseStateStack.top();
					mParseStateStack.pop();
				}
				
				//
				HermitPtr mH_;
				ObjectKeyReceiverPtr mReceiver;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mIsTruncated;
				std::string mLastKey;
				bool mSawListBucketResult;
			};
			
			//
			class Lister {
				//
				class ListCompletion : public S3GetListObjectsXMLCompletion {
				public:
					//
					ListCompletion(const http::HTTPSessionPtr& session,
								   const std::string& awsPublicKey,
								   const std::string& awsSigningKey,
								   const std::string& awsRegion,
								   const std::string& bucketName,
								   const std::string& objectPrefix,
								   const ObjectKeyReceiverPtr& receiver,
								   const S3CompletionBlockPtr& completion) :
					mSession(session),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mBucketName(bucketName),
					mObjectPrefix(objectPrefix),
					mReceiver(receiver),
					mCompletion(completion) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const S3Result& result, const std::string& xml) override {
						if (result == S3Result::kCanceled) {
							mCompletion->Call(h_, S3Result::kCanceled);
							return;
						}
						if (result == S3Result::kTimedOut) {
							mCompletion->Call(h_, S3Result::kTimedOut);
							return;
						}
						if (result != S3Result::kSuccess) {
							NOTIFY_ERROR(h_,
										 "S3ListObjects: S3GetListObjectsXML failed for path:",
										 (std::string(mBucketName) + "/" + std::string(mObjectPrefix)));
							mCompletion->Call(h_, result);
							return;
						}

						ProcessS3ListObjectsXMLClass pc(h_, mReceiver);
						pc.Process(xml);
						if (!pc.mSawListBucketResult) {
							NOTIFY_ERROR(h_, "S3ListObjects: Never saw ListBucketResult tag in response for path:",
											(std::string(mBucketName) + "/" + std::string(mObjectPrefix)));
							mCompletion->Call(h_, S3Result::kError);
							return;
						}

						if (pc.GetIsTruncated() == "true") {
							S3ListObjects(h_,
										  mSession,
										  mAWSPublicKey,
										  mAWSSigningKey,
										  mAWSRegion,
										  mBucketName,
										  mObjectPrefix,
										  pc.GetLastKey(),
										  mReceiver,
										  mCompletion);
							return;
						}
						
						mCompletion->Call(h_, S3Result::kSuccess);
					}
					
					//
					http::HTTPSessionPtr mSession;
					std::string mAWSPublicKey;
					std::string mAWSSigningKey;
					std::string mAWSRegion;
					std::string mBucketName;
					std::string mObjectPrefix;
					ObjectKeyReceiverPtr mReceiver;
					S3CompletionBlockPtr mCompletion;
				};

			public:
				//
				static void S3ListObjects(const HermitPtr& h_,
										  const http::HTTPSessionPtr& session,
										  const std::string& awsPublicKey,
										  const std::string& awsSigningKey,
										  const std::string& awsRegion,
										  const std::string& bucketName,
										  const std::string& objectPrefix,
										  const std::string& marker,
										  const ObjectKeyReceiverPtr& receiver,
										  const S3CompletionBlockPtr& completion) {
					auto listCompletion = std::make_shared<ListCompletion>(session,
																		   awsPublicKey,
																		   awsSigningKey,
																		   awsRegion,
																		   bucketName,
																		   objectPrefix,
																		   receiver,
																		   completion);
					S3GetListObjectsXML(h_,
										session,
										awsPublicKey,
										awsSigningKey,
										awsRegion,
										bucketName,
										objectPrefix,
										marker,
										listCompletion);
				}
			};
			
			
		} // namespace S3ListObjects_Impl
		using namespace S3ListObjects_Impl;
		
		//
		void S3ListObjects(const HermitPtr& h_,
						   const http::HTTPSessionPtr& session,
						   const std::string& awsPublicKey,
						   const std::string& awsSigningKey,
						   const std::string& awsRegion,
						   const std::string& bucketName,
						   const std::string& objectPrefix,
						   const ObjectKeyReceiverPtr& receiver,
						   const S3CompletionBlockPtr& completion) {
			Lister::S3ListObjects(h_,
								  session,
								  awsPublicKey,
								  awsSigningKey,
								  awsRegion,
								  bucketName,
								  objectPrefix,
								  "",
								  receiver,
								  completion);
		}
		
	} // namespace s3
} // namespace hermit
