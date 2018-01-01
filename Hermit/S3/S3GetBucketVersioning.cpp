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
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "S3GetBucketVersioning.h"

namespace hermit {
	namespace s3 {
		namespace S3GetBucketVersioning_Impl {
			
			//
			class ProcessXMLClass : public xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kVersioningConfiguration,
					kStatus,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessXMLClass(const HermitPtr& h_) :
				mH_(h_),
				mParseState(ParseState::kNew),
				mVersioningStatus(S3BucketVersioningStatus::kUnknown) {
				}
				
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "VersioningConfiguration") {
							if (inIsEmptyElement) {
								mVersioningStatus = S3BucketVersioningStatus::kNeverEnabled;
							}
							else {
								PushState(ParseState::kVersioningConfiguration);
							}
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kVersioningConfiguration) {
						if (inStartTag == "Status") {
							PushState(ParseState::kStatus);
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
					if (mParseState == ParseState::kStatus) {
						std::string status = inContent;
						if (status == "Enabled") {
							mVersioningStatus = S3BucketVersioningStatus::kOn;
						}
						else if (status == "Suspended") {
							mVersioningStatus = S3BucketVersioningStatus::kSuspended;
						}
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
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				S3BucketVersioningStatus mVersioningStatus;
			};
			
			//
			class CommandCompletion : public SendS3CommandCompletion {
			public:
				//
				CommandCompletion(const std::string& url,
								  const std::string& bucketName,
								  const S3GetBucketVersioningCompletionPtr& completion) :
				mURL(url),
				mBucketName(bucketName),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const S3Result& result,
								  const S3ParamVector& params,
								  const DataBuffer& data) override {
					if (result != S3Result::kSuccess) {
						if (result == S3Result::k404EntityNotFound) {
							mCompletion->Call(h_, S3Result::k404NoSuchBucket, S3BucketVersioningStatus::kUnknown);
							return;
						}
						NOTIFY_ERROR(h_, "S3GetBucketVersioning: SendS3Command failed for URL:", mURL);
						mCompletion->Call(h_, S3Result::kError, S3BucketVersioningStatus::kUnknown);
						return;
					}
		
					ProcessXMLClass pc(h_);
					std::string response(data.first, data.second);
					pc.Process(response);
					bool success = (pc.mVersioningStatus != S3BucketVersioningStatus::kUnknown);
					if (!success) {
						NOTIFY_ERROR(h_, "S3GetBucketVersioning: couldn't parse XML response for bucket:", mBucketName);
						NOTIFY_ERROR(h_, "response:", response);
						mCompletion->Call(h_, S3Result::kError, S3BucketVersioningStatus::kUnknown);
						return;
					}

					mCompletion->Call(h_, S3Result::kSuccess, pc.mVersioningStatus);
				}
				
				//
				std::string mURL;
				std::string mBucketName;
				S3GetBucketVersioningCompletionPtr mCompletion;
			};

		} // namespace S3GetBucketVersioning_Impl
		using namespace S3GetBucketVersioning_Impl;
		
		//
		void S3GetBucketVersioning(const HermitPtr& h_,
								   const std::string& bucketName,
								   const std::string& s3PublicKey,
								   const std::string& s3PrivateKey,
								   const S3GetBucketVersioningCompletionPtr& completion) {
			std::string method("GET");
			std::string contentType;
			std::string urlToSign("/");
			urlToSign += bucketName;
			urlToSign += "/?versioning";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(s3PublicKey,
								   s3PrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess) {
				NOTIFY_ERROR(h_, "S3GetBucketVersioning: SignAWSRequestVersion2 failed for URL:", urlToSign);
				completion->Call(h_, S3Result::kError, S3BucketVersioningStatus::kUnknown);
				return;
			}
			
			S3ParamVector params;
			params.push_back(std::make_pair("Date", authCallback.mDateString));
			params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += bucketName;
			url += ".s3.amazonaws.com/?versioning";
			
			auto commandCompletion = std::make_shared<CommandCompletion>(url, bucketName, completion);
			SendS3Command(h_, url, method, params, commandCompletion);
		}
		
	} // namespace s3
} // namespace hermit
