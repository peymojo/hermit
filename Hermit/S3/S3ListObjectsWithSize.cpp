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

#include <stack>
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/StringToUInt64.h"
#include "Hermit/XML/ParseXMLData.h"
#include "S3GetListObjectsXML.h"
#include "S3ListObjectsWithSize.h"

namespace hermit {
	namespace s3 {
		namespace S3ListObjectsWithSize_Impl {

			//
			class ProcessS3ListObjectsWithSizeXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kListBucketResult,
					kIsTruncated,
					kContents,
					kKey,
					kSize,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessS3ListObjectsWithSizeXMLClass(const HermitPtr& h_,
													 const ObjectKeyAndSizeReceiverPtr& receiver) :
				mH_(h_),
				mReceiver(receiver),
				mParseState(ParseState::kNew) {
				}
								
				//
				xml::ParseXMLStatus Process(const HermitPtr& h_, const std::string& inXMLData) {
					return xml::ParseXMLData(h_, inXMLData, *this);
				}
				
				//
				std::string GetIsTruncated() const {
					return mIsTruncated;
				}
				
				//
				std::string GetLastKey() const {
					return mLastKey;
				}
				
			private:
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "ListBucketResult") {
							PushState(ParseState::kListBucketResult);
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kListBucketResult) {
						if (inStartTag == "Contents") {
							PushState(ParseState::kContents);
							mPath.clear();
							mSize = 0;
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
						else if (inStartTag == "Size") {
							PushState(ParseState::kSize);
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
						mPath = inContent;
						
						std::string key = inContent;
						if (mLastKey.empty() || (key > mLastKey)) {
							mLastKey = key;
						}
					}
					else if (mParseState == ParseState::kSize) {
						mSize = string::StringToUInt64(inContent);
					}
					else if (mParseState == ParseState::kIsTruncated) {
						mIsTruncated = inContent;
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					if (mParseState == ParseState::kContents) {
						if (!mReceiver->OnOneKeyAndSize(mH_, mPath, mSize)) {
							return xml::kParseXMLStatus_Cancel;
						}
					}
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
				ObjectKeyAndSizeReceiverPtr mReceiver;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mPath;
				uint64_t mSize;
				std::string mIsTruncated;
				std::string mLastKey;
			};
			
			class Lister {
				//
				class GetListXMLCompletion : public S3GetListObjectsXMLCompletion {
				public:
					//
					GetListXMLCompletion(const http::HTTPSessionPtr& session,
										 const std::string& awsPublicKey,
										 const std::string& awsSigningKey,
										 const std::string& awsRegion,
										 const std::string& bucketName,
										 const ObjectKeyAndSizeReceiverPtr& receiver,
										 const S3CompletionBlockPtr& completion) :
					mSession(session),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mBucketName(bucketName),
					mReceiver(receiver),
					mCompletion(completion) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const S3Result& result, const std::string& xml) override {
						if (result == S3Result::kCanceled) {
							mCompletion->Call(h_, result);
							return;
						}
						if (result != S3Result::kSuccess) {
							NOTIFY_ERROR(h_, "S3ListObjectsWithSize: S3GetListObjectsXML failed.");
							mCompletion->Call(h_, result);
							return;
						}
						ProcessS3ListObjectsWithSizeXMLClass pc(h_, mReceiver);
						pc.Process(h_, xml);

						if (pc.GetIsTruncated() == "true") {
							std::string marker(pc.GetLastKey());
							S3ListObjectsWithSize(h_,
												  mSession,
												  mAWSPublicKey,
												  mAWSSigningKey,
												  mAWSRegion,
												  mBucketName,
												  marker,
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
					ObjectKeyAndSizeReceiverPtr mReceiver;
					S3CompletionBlockPtr mCompletion;
				};

			public:
				static void S3ListObjectsWithSize(const HermitPtr& h_,
												  const http::HTTPSessionPtr& session,
												  const std::string& awsPublicKey,
												  const std::string& awsSigningKey,
												  const std::string& awsRegion,
												  const std::string& bucketName,
												  const std::string& marker,
												  const ObjectKeyAndSizeReceiverPtr& receiver,
												  const S3CompletionBlockPtr& completion) {
					auto listCompletion = std::make_shared<GetListXMLCompletion>(session,
																				 awsPublicKey,
																				 awsSigningKey,
																				 awsRegion,
																				 bucketName,
																				 receiver,
																				 completion);
					S3GetListObjectsXML(h_,
										session,
										awsPublicKey,
										awsSigningKey,
										awsRegion,
										bucketName,
										"",
										marker,
										listCompletion);
				}
			};
			
		} // namespace S3ListObjectsWithSize_Impl
		using namespace S3ListObjectsWithSize_Impl;
		
		//
		void S3ListObjectsWithSize(const HermitPtr& h_,
								   const http::HTTPSessionPtr& session,
								   const std::string& awsPublicKey,
								   const std::string& awsSigningKey,
								   const std::string& awsRegion,
								   const std::string& bucketName,
								   const ObjectKeyAndSizeReceiverPtr& receiver,
								   const S3CompletionBlockPtr& completion) {
			Lister::S3ListObjectsWithSize(h_,
										  session,
										  awsPublicKey,
										  awsSigningKey,
										  awsRegion,
										  bucketName,
										  "",
										  receiver,
										  completion);
		}
		
	} // namespace s3
} // namespace hermit
