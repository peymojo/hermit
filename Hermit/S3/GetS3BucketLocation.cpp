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
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/XML/ParseXMLData.h"
#include "GenerateAWS4SigningKey.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "GetS3BucketLocation.h"

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
					if ((inData.first != nullptr) && (inData.second > 0))
					{
						mResponse.assign(inData.first, inData.second);
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponse;
			};
			
			//
			//
			class ProcessErrorXMLClass : xml::ParseXMLClient
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_Error,
					kParseState_Code,
					kParseState_Region,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessErrorXMLClass(const HermitPtr& h_)
				:
				mH_(h_),
				mParseState(kParseState_New)
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
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "Error")
						{
							PushState(kParseState_Error);
						}
					}
					else if (mParseState == kParseState_Error)
					{
						if (inStartTag == "Region")
						{
							if (inIsEmptyElement)
							{
								mLocation = "us-east-1";
							}
							else
							{
								PushState(kParseState_Region);
							}
						}
						else if (inStartTag == "Code")
						{
							PushState(kParseState_Code);
						}
						else if (inStartTag != "?xml")
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
					if (mParseState == kParseState_Region)
					{
						mLocation = inContent;
					}
					else if (mParseState == kParseState_Code)
					{
						mCode = inContent;
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
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mCode;
				std::string mLocation;
			};
			
			//
			//
			class ProcessXMLClass : xml::ParseXMLClient
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_LocationConstraint,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessXMLClass(const HermitPtr& h_)
				:
				mH_(h_),
				mParseState(kParseState_New),
				mFoundLocation(false)
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
						if (inStartTag == "LocationConstraint") {
							if (inIsEmptyElement) {
								mLocation = "us-east-1";
								mFoundLocation = true;
							}
							else {
								PushState(kParseState_LocationConstraint);
							}
						}
						else if (inStartTag != "?xml") {
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
					if (mParseState == kParseState_LocationConstraint) {
						mLocation = inContent;
						mFoundLocation = true;
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
				bool mFoundLocation;
				std::string mLocation;
			};
			
		} // private namespace
		
		//
		void GetS3BucketLocation(const HermitPtr& h_,
								 const std::string& inBucketName,
								 const std::string& inAWSPublicKey,
								 const std::string& inAWSPrivateKey,
								 const GetS3BucketLocationCallbackRef& inCallback) {
			//	Default to us-east-1 for the region since we don't know the region. See the
			//	comment in the error handling case below...
			std::string region("us-east-1");
			std::string method("GET");
			
			std::string host("s3.amazonaws.com");
			std::string s3Path("/");
			s3Path += inBucketName;
			
			time_t now;
			time(&now);
			tm globalTime;
			gmtime_r(&now, &globalTime);
			char dateBuf[64];
			strftime(dateBuf, 64, "%Y%m%dT%H%M%SZ", &globalTime);
			std::string dateTime(dateBuf);
			std::string date(dateTime.substr(0, 8));
			
			GenerateAWS4SigningKeyCallbackClass keyCallback;
			GenerateAWS4SigningKey(inAWSPrivateKey, region, keyCallback);
			std::string signingKey(keyCallback.mKey);
			
			std::string contentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
			
			std::string canonicalRequest(method);
			canonicalRequest += "\n";
			canonicalRequest += s3Path;
			canonicalRequest += "\n";
			canonicalRequest += "location=";
			canonicalRequest += "\n";
			canonicalRequest += "host:";
			canonicalRequest += host;
			canonicalRequest += "\n";
			canonicalRequest += "x-amz-content-sha256:";
			canonicalRequest += contentSHA256;
			canonicalRequest += "\n";
			canonicalRequest += "x-amz-date:";
			canonicalRequest += dateTime;
			canonicalRequest += "\n";
			canonicalRequest += "\n";
			canonicalRequest += "host;x-amz-content-sha256;x-amz-date";
			canonicalRequest += "\n";
			canonicalRequest += contentSHA256;
			
			std::string canonicalRequestSHA256;
			encoding::CalculateSHA256(canonicalRequest, canonicalRequestSHA256);
			std::string canonicalRequestSHA256Hex;
			string::BinaryStringToHex(canonicalRequestSHA256, canonicalRequestSHA256Hex);
			
			std::string stringToSign("AWS4-HMAC-SHA256");
			stringToSign += "\n";
			stringToSign += dateTime;
			stringToSign += "\n";
			stringToSign += date;
			stringToSign += "/";
			stringToSign += region;
			stringToSign += "/s3/aws4_request";
			stringToSign += "\n";
			stringToSign += canonicalRequestSHA256Hex;
			
			std::string stringToSignSHA256;
			encoding::CalculateHMACSHA256(signingKey, stringToSign, stringToSignSHA256);
			std::string stringToSignSHA256Hex;
			string::BinaryStringToHex(stringToSignSHA256, stringToSignSHA256Hex);
			
			std::string authorization("AWS4-HMAC-SHA256 Credential=");
			authorization += inAWSPublicKey;
			authorization += "/";
			authorization += date;
			authorization += "/";
			authorization += region;
			authorization += "/s3/aws4_request";
			authorization += ",";
			authorization += "SignedHeaders=host;x-amz-content-sha256;x-amz-date";
			authorization += ",";
			authorization += "Signature=";
			authorization += stringToSignSHA256Hex;
			
			StringPairVector params;
			params.push_back(StringPair("x-amz-date", dateTime));
			params.push_back(StringPair("x-amz-content-sha256", contentSHA256));
			params.push_back(StringPair("Authorization", authorization));
			
			std::string url("https://");
			url += host;
			url += s3Path;
			url += "?location";
			
			EnumerateStringValuesFunctionClass headerParams(params);
			SendCommandCallback result;
			SendS3Command(h_, url, method, headerParams, result);
			if (result.mStatus == S3Result::kCanceled) {
				inCallback.Call(S3Result::kCanceled, "");
				return;
			}
			if (result.mStatus != S3Result::kSuccess) {
				//	UPDATE: This code path should no longer be necessary as we've switched to using the
				//	path-style URL for this request which doesn't forward to the bucket's actual region
				//	based on advice from the AWS team. Leaving it here just in case though...
				//
				//	OK, so this is a bit surreal... but under AWS4 authentication, you need the region
				//	to sign requests... which we don't know because this is the get location (aka region) call!?
				//	BUT the error response will tell us which reason to use. So... Um... we'll run with it?
				if ((result.mStatus == S3Result::kAuthorizationHeaderMalformed) &&
					!result.mResponse.empty()) {
					ProcessErrorXMLClass pc(h_);
					pc.Process(result.mResponse);
					if (!pc.mLocation.empty()) {
						inCallback.Call(S3Result::kSuccess, pc.mLocation);
					}
					else {
						NOTIFY_ERROR(h_,
									 "GetS3BucketLocation: S3Result::kAuthorizationHeaderMalformed but location couldn't be determined, response:",
									 result.mResponse);
						
						inCallback.Call(S3Result::kError, "");
					}
					return;
				}
				
				if (result.mStatus == S3Result::k404NoSuchBucket) {
					inCallback.Call(S3Result::k404NoSuchBucket, "");
				}
				else if (result.mStatus == S3Result::kTimedOut) {
					inCallback.Call(S3Result::kTimedOut, "");
				}
				else if (result.mStatus == S3Result::kNetworkConnectionLost) {
					inCallback.Call(S3Result::kNetworkConnectionLost, "");
				}
				else if (result.mStatus == S3Result::kNoNetworkConnection) {
					inCallback.Call(S3Result::kNoNetworkConnection, "");
				}
				else if ((result.mStatus == S3Result::kS3InternalError) ||
						 (result.mStatus == S3Result::k500InternalServerError) ||
						 (result.mStatus == S3Result::k503ServiceUnavailable)) {
					inCallback.Call(result.mStatus, "");
				}
				else {
					ProcessErrorXMLClass pc(h_);
					pc.Process(result.mResponse);
					if (pc.mCode == "InvalidAccessKeyId") {
						NOTIFY_ERROR(h_, "GetS3BucketLocation: InvalidAccessKeyId");
						inCallback.Call(S3Result::kInvalidAccessKey, "");
					}
					else if (pc.mCode == "SignatureDoesNotMatch") {
						NOTIFY_ERROR(h_, "GetS3BucketLocation: SignatureDoesNotMatch");
						//				NOTIFY_ERROR("-- response:", result.mResponse);
						inCallback.Call(S3Result::kSignatureDoesNotMatch, "");
					}
					else if (pc.mCode == "AccessDenied") {
						NOTIFY_ERROR(h_, "GetS3BucketLocation: AccessDenied");
						inCallback.Call(S3Result::k403AccessDenied, "");
					}
					else {
						NOTIFY_ERROR(h_, "GetS3BucketLocation: SendS3Command failed for URL:", url);
						NOTIFY_ERROR(h_, "-- response:", result.mResponse);
						inCallback.Call(S3Result::kError, "");
					}
				}
			}
			else {
				//		Log(kLogLevel_Error, result.mResponse);
				
				ProcessXMLClass pc(h_);
				pc.Process(result.mResponse);
				if (!pc.mFoundLocation) {
					NOTIFY_ERROR(h_, "GetS3BucketLocation: couldn't find location in XML response for bucket:", inBucketName);
					NOTIFY_ERROR(h_, "-- response:", result.mResponse);
					inCallback.Call(S3Result::kError, "");
				}
				else {
					inCallback.Call(S3Result::kSuccess, pc.mLocation);
				}
			}
		}
		
	} // namespace s3
} // namespace hermit
