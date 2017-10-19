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
		
		namespace
		{
			//
			//
			class ProcessS3ListObjectsXMLClass : xml::ParseXMLClient
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_ListBucketResult,
					kParseState_IsTruncated,
					kParseState_Contents,
					kParseState_Key,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessS3ListObjectsXMLClass(const HermitPtr& h_, ObjectKeyReceiver& receiver)
				:
				mH_(h_),
				mReceiver(receiver),
				mParseState(kParseState_New),
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
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "ListBucketResult")
						{
							mSawListBucketResult = true;
							PushState(kParseState_ListBucketResult);
						}
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_ListBucketResult)
					{
						if (inStartTag == "Contents")
						{
							PushState(kParseState_Contents);
						}
						else if (inStartTag == "IsTruncated")
						{
							PushState(kParseState_IsTruncated);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Contents)
					{
						if (inStartTag == "Key")
						{
							PushState(kParseState_Key);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else
					{
						PushState(kParseState_IgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == kParseState_Key)
					{
						//				std::cout << "ParseXMLOnContent: inContent=|" << inContent << "|\n";
						//				std::cout << inContent << "\n";
						
						std::string key = inContent;
						if (mLastKey.empty() || (key > mLastKey))
						{
							mLastKey = key;
						}
						
						if (!mReceiver(inContent)) {
							return xml::kParseXMLStatus_Cancel;
						}
					}
					else if (mParseState == kParseState_IsTruncated)
					{
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
				//
				void PushState(ParseState inNewState)
				{
					mParseStateStack.push(mParseState);
					mParseState = inNewState;
				}
				
				//
				//
				void PopState()
				{
					mParseState = mParseStateStack.top();
					mParseStateStack.pop();
				}
				
				//
				//
				HermitPtr mH_;
				ObjectKeyReceiver& mReceiver;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mIsTruncated;
				std::string mLastKey;
				bool mSawListBucketResult;
			};
			
			//
			class OnListObjectsXMLClass : public S3GetListObjectsXMLCallback {
			public:
				//
				OnListObjectsXMLClass(const HermitPtr& h_, ObjectKeyReceiver& receiver) :
				mH_(h_),
				mReceiver(receiver),
				mResult(S3Result::kUnknown),
				mIsTruncated(false),
				mSawListBucketResult(false) {
				}
				
				//
				virtual bool Function(const HermitPtr& h_, const S3Result& inResult, const std::string& inXML) override {
					mResult = inResult;
					if (inResult == S3Result::kSuccess) {
						ProcessS3ListObjectsXMLClass pc(mH_, mReceiver);
						pc.Process(inXML);
						
						mSawListBucketResult = pc.mSawListBucketResult;
						mIsTruncated = (pc.GetIsTruncated() == "true");
						if (mIsTruncated)
						{
							mLastKey = pc.GetLastKey();
						}
					}
					return true;
				}
				
				//
				HermitPtr mH_;
				ObjectKeyReceiver& mReceiver;
				S3Result mResult;
				bool mIsTruncated;
				std::string mLastKey;
				bool mSawListBucketResult;
			};
			
		} // private namespace
		
		//
		S3Result S3ListObjects(const HermitPtr& h_,
							   const std::string& awsPublicKey,
							   const std::string& awsSigningKey,
							   const std::string& awsRegion,
							   const std::string& bucketName,
							   const std::string& objectPrefix,
							   ObjectKeyReceiver& receiver) {
		
			std::string marker;
			while (true) {
				OnListObjectsXMLClass callback(h_, receiver);
				S3GetListObjectsXML(h_,
									awsPublicKey,
									awsSigningKey,
									awsRegion,
									bucketName,
									objectPrefix,
									marker,
									callback);
				
				if (callback.mResult == S3Result::kCanceled) {
					return S3Result::kCanceled;
				}
				if (callback.mResult == S3Result::kTimedOut) {
					return S3Result::kTimedOut;
				}
				if (callback.mResult != S3Result::kSuccess) {
					NOTIFY_ERROR(h_,
								 "S3ListObjects: S3GetListObjectsXML failed for path:",
								 (std::string(bucketName) + "/" + std::string(objectPrefix)));
					
					return callback.mResult;
				}
				
				if (!callback.mSawListBucketResult) {
					NOTIFY_ERROR(h_, "S3ListObjects: Never saw ListBucketResult tag in response for path:",
								 (std::string(bucketName) + "/" + std::string(objectPrefix)));
					
					return S3Result::kError;
				}
				
				if (!callback.mIsTruncated) {
					break;
				}
				marker = callback.mLastKey;
			}
			
			return S3Result::kSuccess;
		}
		
	} // namespace s3
} // namespace hermit
