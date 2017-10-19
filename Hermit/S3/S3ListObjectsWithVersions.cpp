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
#include "S3GetListObjectsWithVersionsXML.h"
#include "S3ListObjectsWithVersions.h"

namespace hermit {
	namespace s3 {
		
		//
		//
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
					kParseState_ListVersionsResult,
					kParseState_IsTruncated,
					kParseState_Version,
					kParseState_Key,
					kParseState_VersionId,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
								
			public:
				//
				ProcessS3ListObjectsXMLClass(const S3ListObjectsWithVersionsCallbackRef& inCallback)
				:
				mCallback(inCallback),
				mParseState(kParseState_New),
				mDeleteMarker(false),
				mAtLeastOneItemFound(false)
				{
				}
				
				//
				xml::ParseXMLStatus Process(const HermitPtr& h_, const std::string& inXMLData) {
					return xml::ParseXMLData(h_, inXMLData, *this);
				}
				
				//
				std::string GetIsTruncated() const
				{
					return mIsTruncated;
				}
				
				//
				std::string GetLastKey() const
				{
					return mLastKey;
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "ListVersionsResult")
						{
							PushState(kParseState_ListVersionsResult);
						}
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_ListVersionsResult)
					{
						if (inStartTag == "Version")
						{
							PushState(kParseState_Version);
							mKey.clear();
							mVersionID.clear();
							mDeleteMarker = false;
						}
						else if (inStartTag == "DeleteMarker")
						{
							PushState(kParseState_Version);
							mKey.clear();
							mVersionID.clear();
							mDeleteMarker = true;
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
					else if (mParseState == kParseState_Version)
					{
						if (inStartTag == "Key")
						{
							PushState(kParseState_Key);
						}
						else if (inStartTag == "VersionId")
						{
							PushState(kParseState_VersionId);
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
						mKey = inContent;
						
						if (mLastKey.empty() || (mKey > mLastKey))
						{
							mLastKey = mKey;
						}
					}
					else if (mParseState == kParseState_VersionId)
					{
						mVersionID = inContent;
					}
					else if (mParseState == kParseState_IsTruncated)
					{
						mIsTruncated = inContent;
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					if (mParseState == kParseState_Version)
					{
						if (!mKey.empty() && !mVersionID.empty())
						{
							mAtLeastOneItemFound = true;
							mCallback.Call(
										   kS3ListObjectsWithVersionsStatus_Success,
										   mKey,
										   mVersionID,
										   mDeleteMarker);
						}
					}
					PopState();
					return xml::kParseXMLStatus_OK;
				}
				
				//
				//
				void PushState(
							   ParseState inNewState)
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
				const S3ListObjectsWithVersionsCallbackRef& mCallback;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mKey;
				std::string mVersionID;
				bool mDeleteMarker;
				std::string mIsTruncated;
				std::string mLastKey;
				bool mAtLeastOneItemFound;
			};
			
			//
			//
			class OnListObjectsWithVersionsXMLClass
			:
			public S3GetListObjectsWithVersionsXMLCallback
			{
			public:
				//
				//
				OnListObjectsWithVersionsXMLClass(const S3ListObjectsWithVersionsCallbackRef& inCallback)
				:
				mCallback(inCallback),
				mSuccess(false),
				mIsTruncated(false),
				mAtLeastOneItemFound(false)
				{
				}
				
				//
				//
				bool Function(const HermitPtr& h_, const bool& inSuccess, const std::string& inXML) {
					mSuccess = inSuccess;
					if (inSuccess) {
						ProcessS3ListObjectsXMLClass pc(mCallback);
						pc.Process(h_, inXML);
						
						mIsTruncated = (pc.GetIsTruncated() == "true");
						if (mIsTruncated) {
							mLastKey = pc.GetLastKey();
						}
						mAtLeastOneItemFound = pc.mAtLeastOneItemFound;
					}
					return true;
				}
				
				//
				//
				const S3ListObjectsWithVersionsCallbackRef& mCallback;
				bool mSuccess;
				bool mIsTruncated;
				std::string mLastKey;
				bool mAtLeastOneItemFound;
			};
			
		} // private namespace
		
		//
		void S3ListObjectsWithVersions(const HermitPtr& h_,
									   const std::string& inAWSPublicKey,
									   const std::string& inAWSSigningKey,
									   const uint64_t& inAWSSigningKeySize,
									   const std::string& inAWSRegion,
									   const std::string& inBucketName,
									   const std::string& inRootPath,
									   const S3ListObjectsWithVersionsCallbackRef& inCallback) {
			
			bool atLeastOneItemFound = false;
			std::string marker;
			while (true) {
				OnListObjectsWithVersionsXMLClass callback(inCallback);
				S3GetListObjectsWithVersionsXML(h_,
												inAWSPublicKey,
												inAWSSigningKey,
												inAWSSigningKeySize,
												inAWSRegion,
												inBucketName,
												inRootPath,
												marker,
												callback);
				
				if (!callback.mSuccess) {
					NOTIFY_ERROR(h_,
								 "S3ListObjectsWithVersions: S3GetListObjectsWithVersionsXML failed for prefix:",
								 inRootPath);
					
					inCallback.Call(kS3ListObjectsWithVersionsStatus_Error, "", "", false);
					break;
				}
				if (callback.mAtLeastOneItemFound) {
					atLeastOneItemFound = true;
				}
				if (!callback.mIsTruncated) {
					break;
				}
				marker = callback.mLastKey;
			}
			
			if (!atLeastOneItemFound) {
				inCallback.Call(kS3ListObjectsWithVersionsStatus_NoObjectsFound, "", "", false);
			}
		}
		
	} // namespace s3
} // namespace hermit

