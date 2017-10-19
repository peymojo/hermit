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
		
		namespace
		{
			//
			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			//
			class SendCommandCallback
			:
			public SendS3CommandCallback
			{
			public:
				//
				//
				SendCommandCallback()
				:
				mStatus(S3Result::kUnknown)
				{
				}
				
				//
				//
				bool Function(
							  const S3Result& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const DataBuffer& inData)
				{
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess)
					{
						mResponse = std::string(inData.first, inData.second);
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponse;
			};
			
			//
			class ProcessXMLClass : public xml::ParseXMLClient {
			private:
				//
				enum ParseState {
					kParseState_New,
					kParseState_VersioningConfiguration,
					kParseState_Status,
					kParseState_IgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessXMLClass(const HermitPtr& h_)
				:
				mH_(h_),
				mParseState(kParseState_New),
				mVersioningStatus(S3BucketVersioningStatus::kUnknown)
				{
				}
				
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					
					if (mParseState == kParseState_New) {
						if (inStartTag == "VersioningConfiguration") {
							if (inIsEmptyElement) {
								mVersioningStatus = S3BucketVersioningStatus::kNeverEnabled;
							}
							else {
								PushState(kParseState_VersioningConfiguration);
							}
						}
						else if (inStartTag != "?xml") {
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_VersioningConfiguration) {
						if (inStartTag == "Status") {
							PushState(kParseState_Status);
						}
						else {
							PushState(kParseState_IgnoredElement);
						}
					}
					else {
						PushState(kParseState_IgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == kParseState_Status) {
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
				//
				HermitPtr mH_;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				S3BucketVersioningStatus mVersioningStatus;
			};
			
		} // private namespace
		
		//
		//
		void S3GetBucketVersioning(const HermitPtr& h_,
								   const std::string& inBucketName,
								   const std::string& inS3PublicKey,
								   const std::string& inS3PrivateKey,
								   const S3GetBucketVersioningCallbackRef& inCallback)
		{
			std::string method("GET");
			std::string contentType;
			std::string urlToSign("/");
			urlToSign += inBucketName;
			urlToSign += "/?versioning";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(
								   inS3PublicKey,
								   inS3PrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess)
			{
				NOTIFY_ERROR(h_, "S3GetBucketVersioning: SignAWSRequestVersion2 failed for URL:", urlToSign);
				inCallback.Call(S3Result::kError, S3BucketVersioningStatus::kUnknown);
				return;
			}
			
			StringPairVector params;
			params.push_back(StringPair("Date", authCallback.mDateString));
			params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += inBucketName;
			url += ".s3.amazonaws.com/?versioning";
			
			EnumerateStringValuesFunctionClass headerParams(params);
			SendCommandCallback result;
			SendS3Command(h_, url, method, headerParams, result);
			if (result.mStatus != S3Result::kSuccess) {
				if (result.mStatus == S3Result::k404EntityNotFound) {
					inCallback.Call(S3Result::k404NoSuchBucket, S3BucketVersioningStatus::kUnknown);
				}
				else {
					NOTIFY_ERROR(h_, "S3GetBucketVersioning: SendS3Command failed for URL:", url);
					inCallback.Call(S3Result::kError, S3BucketVersioningStatus::kUnknown);
				}
			}
			else {
				//Log(inHub, kLogLevel_Error, result.mResponse);
				
				ProcessXMLClass pc(h_);
				pc.Process(result.mResponse);
				
				bool success = (pc.mVersioningStatus != S3BucketVersioningStatus::kUnknown);
				if (!success) {
					NOTIFY_ERROR(h_, "S3GetBucketVersioning: couldn't parse XML response for bucket:", inBucketName);
					NOTIFY_ERROR(h_, "response:", result.mResponse);
					inCallback.Call(S3Result::kError, S3BucketVersioningStatus::kUnknown);
				}
				else {
					inCallback.Call(S3Result::kSuccess, pc.mVersioningStatus);
				}
			}
		}
		
	} // namespace s3
} // namespace hermit
