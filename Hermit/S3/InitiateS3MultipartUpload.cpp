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
#include "SendS3Command.h"
#include "InitiateS3MultipartUpload.h"

namespace hermit {
	namespace s3 {
		
		namespace {
			
			//
			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			//
			class ParamCallback
			:
			public EnumerateStringValuesCallback
			{
			public:
				//
				//
				bool Function(const std::string& inName, const std::string& inValue)
				{
					if (inName == "Endpoint")
					{
						mNewEndpoint = inValue;
					}
					return true;
				}
				
				//
				//
				std::string mNewEndpoint;
			};
			
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
						mResponseData = std::string(inData.first, inData.second);
					}
					else if (inStatus == S3Result::k307TemporaryRedirect)
					{
						ParamCallback callback;
						inParamFunction.Call(callback);
						mNewEndpoint = callback.mNewEndpoint;
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponseData;
				std::string mNewEndpoint;
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
					kParseState_InitiateMultipartUploadResult,
					kParseState_UploadId,
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
						if (inStartTag == "InitiateMultipartUploadResult")
						{
							PushState(kParseState_InitiateMultipartUploadResult);
						}
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_InitiateMultipartUploadResult)
					{
						if (inStartTag == "UploadId")
						{
							PushState(kParseState_UploadId);
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
					if (mParseState == kParseState_UploadId)
					{
						mUploadId = inContent;
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
				std::string mUploadId;
			};
			
		} // private namespace
		
		//
		//
		void InitiateS3MultipartUpload(const HermitPtr& h_,
									   const std::string& inAWSPublicKey,
									   const std::string& inAWSSigningKey,
									   const uint64_t& inAWSSigningKeySize,
									   const std::string& inAWSRegion,
									   const std::string& inS3BucketName,
									   const std::string& inS3ObjectKey,
									   const std::string& inDataSHA256Hex,
									   const InitiateS3MultipartUploadCallbackRef& inCallback)
		{
			std::string region(inAWSRegion);
			std::string method("POST");
			
			std::string host(inS3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string s3Path(inS3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/'))
			{
				s3Path.insert(0, "/");
			}
			
			int redirectCount = 0;
			while (true)
			{
				time_t now;
				time(&now);
				tm globalTime;
				gmtime_r(&now, &globalTime);
				char dateBuf[2048];
				strftime(dateBuf, 2048, "%Y%m%dT%H%M%SZ", &globalTime);
				std::string dateTime(dateBuf);
				std::string date(dateTime.substr(0, 8));
				
				std::string contentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
				
				std::string canonicalRequest(method);
				canonicalRequest += "\n";
				canonicalRequest += s3Path;
				canonicalRequest += "\n";
				canonicalRequest += "uploads=";
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
				canonicalRequest += "x-amz-meta-sha256:";
				canonicalRequest += inDataSHA256Hex;
				canonicalRequest += "\n";
				canonicalRequest += "\n";
				canonicalRequest += "host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
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
				encoding::CalculateHMACSHA256(inAWSSigningKey, stringToSign, stringToSignSHA256);
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
				authorization += "SignedHeaders=host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
				authorization += ",";
				authorization += "Signature=";
				authorization += stringToSignSHA256Hex;
				
				StringPairVector params;
				params.push_back(StringPair("x-amz-date", dateTime));
				params.push_back(StringPair("x-amz-content-sha256", contentSHA256));
				params.push_back(StringPair("x-amz-meta-sha256", inDataSHA256Hex));
				params.push_back(StringPair("Authorization", authorization));
				
				std::string url("https://");
				url += host;
				url += s3Path;
				url += "?uploads";
				
				EnumerateStringValuesFunctionClass headerParams(params);
				SendCommandCallback result;
				SendS3Command(h_, url, method, headerParams, result);
				if (result.mStatus == S3Result::kCanceled) {
					inCallback.Call(result.mStatus, "");
					return;
				}
				if (result.mStatus == S3Result::k307TemporaryRedirect) {
					if (result.mNewEndpoint.empty()) {
						NOTIFY_ERROR(h_,
									 "InitiateS3MultipartUpload: S3Result::k307TemporaryRedirect but new endpoint is empty for url:",
									 url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (result.mNewEndpoint == host) {
						NOTIFY_ERROR(h_,
									 "InitiateS3MultipartUpload: S3Result::k307TemporaryRedirect but new endpoint is the same for url:",
									 url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (++redirectCount >= 5) {
						NOTIFY_ERROR(h_, "InitiateS3MultipartUpload: too many temporary redirects for url:", url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					host = result.mNewEndpoint;
					continue;
				}
				if (result.mStatus == S3Result::kTimedOut) {
					inCallback.Call(S3Result::kTimedOut, "");
					return;
				}
				if (result.mStatus == S3Result::kNetworkConnectionLost) {
					inCallback.Call(S3Result::kNetworkConnectionLost, "");
					return;
				}
				if (result.mStatus == S3Result::kNoNetworkConnection) {
					inCallback.Call(S3Result::kNoNetworkConnection, "");
					return;
				}
				if (result.mStatus == S3Result::k403AccessDenied) {
					inCallback.Call(result.mStatus, "");
					return;
				}
				if ((result.mStatus == S3Result::kS3InternalError) ||
					(result.mStatus == S3Result::k500InternalServerError) ||
					(result.mStatus == S3Result::k503ServiceUnavailable)) {
					inCallback.Call(result.mStatus, "");
					return;
				}
				if (result.mStatus != S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "InitiateS3MultipartUpload: SendS3Command failed for URL:", url);
					inCallback.Call(S3Result::kError, "");
					return;
				}
				
				ProcessXMLClass xmlClass(h_);
				xmlClass.Process(result.mResponseData);
				if (xmlClass.mUploadId.empty()) {
					NOTIFY_ERROR(h_, "InitiateS3MultipartUpload: xmlClass.mUploadId.empty() URL:", url);
					inCallback.Call(S3Result::kError, "");
					return;
				}
				
				inCallback.Call(S3Result::kSuccess, xmlClass.mUploadId);
				break;
			}
		}
		
	} // namespace s3
} // namespace hermit
