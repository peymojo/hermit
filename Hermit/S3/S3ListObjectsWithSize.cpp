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
		
		//
		//
		namespace
		{
			//
			//
			class ProcessS3ListObjectsWithSizeXMLClass : xml::ParseXMLClient
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
					kParseState_Size,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessS3ListObjectsWithSizeXMLClass(
													 const S3ListObjectsWithSizeCallbackRef& inCallback)
				:
				mCallback(inCallback),
				mParseState(kParseState_New)
				{
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
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "ListBucketResult")
						{
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
							mPath.clear();
							mSize = 0;
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
						else if (inStartTag == "Size")
						{
							PushState(kParseState_Size);
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
					if (mParseState == kParseState_Key) {
						mPath = inContent;
						
						std::string key = inContent;
						if (mLastKey.empty() || (key > mLastKey)) {
							mLastKey = key;
						}
					}
					else if (mParseState == kParseState_Size) {
						mSize = string::StringToUInt64(inContent);
					}
					else if (mParseState == kParseState_IsTruncated) {
						mIsTruncated = inContent;
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					if (mParseState == kParseState_Contents) {
						mCallback.Call(S3Result::kSuccess, mPath, mSize);
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
				const S3ListObjectsWithSizeCallbackRef& mCallback;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mPath;
				uint64_t mSize;
				std::string mIsTruncated;
				std::string mLastKey;
			};
			
			//
			class OnListObjectsXMLClass
			:
			public S3GetListObjectsXMLCallback
			{
			public:
				//
				OnListObjectsXMLClass(const S3ListObjectsWithSizeCallbackRef& inCallback)
				:
				mResult(S3Result::kUnknown),
				mCallback(inCallback),
				mIsTruncated(false)
				{
				}
				
				//
				virtual bool Function(const HermitPtr& h_, const S3Result& inResult, const std::string& inXML) override {
					mResult = inResult;
					if (inResult == S3Result::kSuccess) {
						ProcessS3ListObjectsWithSizeXMLClass pc(mCallback);
						pc.Process(h_, inXML);
						
						mIsTruncated = (pc.GetIsTruncated() == "true");
						if (mIsTruncated) {
							mLastKey = pc.GetLastKey();
						}
					}
					return true;
				}
				
				//
				S3Result mResult;
				const S3ListObjectsWithSizeCallbackRef& mCallback;
				bool mIsTruncated;
				std::string mLastKey;
			};
			
		} // private namespace
		
		//
		void S3ListObjectsWithSize(const HermitPtr& h_,
								   const std::string& inAWSPublicKey,
								   const std::string& inAWSSigningKey,
								   const std::string& inAWSRegion,
								   const std::string& inBucketName,
								   const S3ListObjectsWithSizeCallbackRef& inCallback)
		{
			std::string marker;
			while (true) {
				OnListObjectsXMLClass callback(inCallback);
				S3GetListObjectsXML(h_,
									inAWSPublicKey,
									inAWSSigningKey,
									inAWSRegion,
									inBucketName,
									"",
									marker,
									callback);
				
				if (callback.mResult == S3Result::kCanceled) {
					inCallback.Call(callback.mResult, "", 0);
					return;
				}
				if (callback.mResult != S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "S3ListObjectsWithSize: S3GetListObjectsXML failed.");
					inCallback.Call(callback.mResult, "", 0);
					return;
				}
				
				if (!callback.mIsTruncated) {
					break;
				}
				marker = callback.mLastKey;
			}
		}
		
	} // namespace s3
} // namespace hermit
