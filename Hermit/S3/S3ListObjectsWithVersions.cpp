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
		namespace S3ListObjectsWithVersions_Impl {

			//
			class Lister {
				//
				class ProcessS3ListObjectsXMLClass : xml::ParseXMLClient {
				private:
					//
					enum class ParseState {
						kNew,
						kListVersionsResult,
						kIsTruncated,
						kVersion,
						kKey,
						kVersionId,
						kIgnoredElement
					};
					
					//
					typedef std::stack<ParseState> ParseStateStack;
					
				public:
					//
					ProcessS3ListObjectsXMLClass(const HermitPtr& h_, const S3ObjectEnumeratorPtr& enumerator) :
					mH_(h_),
					mEnumerator(enumerator),
					mParseState(ParseState::kNew),
					mDeleteMarker(false),
					mAtLeastOneItemFound(false) {
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
					
					//
					virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
														const std::string& inAttributes,
														bool inIsEmptyElement) override {
						if (mParseState == ParseState::kNew) {
							if (inStartTag == "ListVersionsResult") {
								PushState(ParseState::kListVersionsResult);
							}
							else if (inStartTag != "?xml") {
								PushState(ParseState::kIgnoredElement);
							}
						}
						else if (mParseState == ParseState::kListVersionsResult) {
							if (inStartTag == "Version") {
								PushState(ParseState::kVersion);
								mKey.clear();
								mVersionId.clear();
								mDeleteMarker = false;
							}
							else if (inStartTag == "DeleteMarker") {
								PushState(ParseState::kVersion);
								mKey.clear();
								mVersionId.clear();
								mDeleteMarker = true;
							}
							else if (inStartTag == "IsTruncated") {
								PushState(ParseState::kIsTruncated);
							}
							else {
								PushState(ParseState::kIgnoredElement);
							}
						}
						else if (mParseState == ParseState::kVersion) {
							if (inStartTag == "Key") {
								PushState(ParseState::kKey);
							}
							else if (inStartTag == "VersionId") {
								PushState(ParseState::kVersionId);
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
							mKey = inContent;
							if (mLastKey.empty() || (mKey > mLastKey)) {
								mLastKey = mKey;
							}
						}
						else if (mParseState == ParseState::kVersionId) {
							mVersionId = inContent;
						}
						else if (mParseState == ParseState::kIsTruncated) {
							mIsTruncated = inContent;
						}
						return xml::kParseXMLStatus_OK;
					}
					
					//
					virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
						if (mParseState == ParseState::kVersion) {
							if (!mKey.empty() && !mVersionId.empty()) {
								mAtLeastOneItemFound = true;
								if (!mEnumerator->OnOneItem(mH_, mKey, mVersionId, mDeleteMarker)) {
									return xml::kParseXMLStatus_Cancel;
								}
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
					S3ObjectEnumeratorPtr mEnumerator;
					ParseState mParseState;
					ParseStateStack mParseStateStack;
					std::string mKey;
					std::string mVersionId;
					bool mDeleteMarker;
					std::string mIsTruncated;
					std::string mLastKey;
					bool mAtLeastOneItemFound;
				};
				
				//
				class OnListObjectsWithVersionsXMLClass : public S3GetListObjectsWithVersionsXMLCompletion {
				public:
					//
					OnListObjectsWithVersionsXMLClass(const S3ObjectEnumeratorPtr& enumerator,
													  const S3CompletionBlockPtr& completion) :
					mEnumerator(enumerator),
					mCompletion(completion),
					mSuccess(false),
					mIsTruncated(false),
					mAtLeastOneItemFound(false) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const bool& inSuccess, const std::string& inXML) override {
						mSuccess = inSuccess;
						if (inSuccess) {
							ProcessS3ListObjectsXMLClass pc(h_, mEnumerator);
							pc.Process(h_, inXML);
							
							mIsTruncated = (pc.GetIsTruncated() == "true");
							if (mIsTruncated) {
								mLastKey = pc.GetLastKey();
							}
							mAtLeastOneItemFound = pc.mAtLeastOneItemFound;
						}
					}
					
					//
					S3ObjectEnumeratorPtr mEnumerator;
					S3CompletionBlockPtr mCompletion;
					bool mSuccess;
					bool mIsTruncated;
					std::string mLastKey;
					bool mAtLeastOneItemFound;
				};

			};
			
		} // namespace S3ListObjectsWithVersions_Impl
		using namespace S3ListObjectsWithVersions_Impl;
		
		//
		void S3ListObjectsWithVersions(const HermitPtr& h_,
									   const std::string& awsPublicKey,
									   const std::string& awsSigningKey,
									   const std::string& awsRegion,
									   const std::string& bucketName,
									   const std::string& pathPrefix,
									   const S3ObjectEnumeratorPtr& enumerator,
									   const S3CompletionBlockPtr& completion) {

			//	Needs to be updated to use background tasks & S3GetListObjectsWithVersionsXML
			// 	needs to be updated.
			NOTIFY_ERROR(h_, "S3ListObjectsWithVersions: not implemented");
			completion->Call(h_, S3Result::kError);
			
#if 000
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
#endif
			
		}
		
	} // namespace s3
} // namespace hermit

