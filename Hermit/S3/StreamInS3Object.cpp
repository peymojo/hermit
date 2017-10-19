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

#include <map>
#include <stack>
#include <string>
#include <vector>
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/HTTP/URLEncode.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SignAWSRequestVersion2.h"
#include "StreamInS3Request.h"
#include "StreamInS3Object.h"

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
			class ProcessXMLClass : xml::ParseXMLClient
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_Error,
					kParseState_Code,
					kParseState_Endpoint,
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
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Error)
					{
						if (inStartTag == "Code")
						{
							PushState(kParseState_Code);
						}
						else if (inStartTag == "Endpoint")
						{
							PushState(kParseState_Endpoint);
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
					if (mParseState == kParseState_Code)
					{
						mCode = inContent;
					}
					else if (mParseState == kParseState_Endpoint)
					{
						mEndpoint = inContent;
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
				std::string mEndpoint;
			};
			
			//
			//
			typedef std::map<std::string, std::string> ParamMap;
			
			//
			//
			class StreamInResult
			:
			public StreamInS3RequestCallback
			{
				//
				//
				class ResponseParams
				:
				public EnumerateStringValuesCallback
				{
				public:
					//
					//
					bool Function(const std::string& inName, const std::string& inValue)
					{
						mParams.insert(ParamMap::value_type(inName, inValue));
						return true;
					}
					
					//
					//
					ParamMap mParams;
				};
				
			public:
				//
				//
				StreamInResult()
				:
				mResult(S3Result::kUnknown)
				{
				}
				
				//
				//
				bool Function(const S3Result& inResult,
							  const EnumerateStringValuesFunctionRef& inParamsFunction)
				{
					mResult = inResult;
					if (inResult == S3Result::kSuccess)
					{
						ResponseParams responseParams;
						if (!inParamsFunction.Call(responseParams))
						{
							return false;
						}
						mParams = responseParams.mParams;
					}
					return true;
				}
				
				//
				//
				S3Result mResult;
				ParamMap mParams;
			};
			
			//
			//
			class StreamInDataHandler
			:
			public StreamInS3RequestDataHandlerFunction
			{
			public:
				//
				//
				StreamInDataHandler()
				{
				}
				
				//
				//
				bool Function(
							  const uint64_t& inExpectedDataSize,
							  const DataBuffer& inDataPart,
							  const bool& inIsEndOfStream)
				{
					if (inDataPart.second > 0)
					{
						mResponse += std::string(inDataPart.first, inDataPart.second);
					}
					return true;
				}
				
				//
				//
				std::string mResponse;
			};
			
		} // private namespace
		
		//
		S3Result StreamInS3Object(const HermitPtr& h_,
								  const std::string& inAWSPublicKey,
								  const std::string& inAWSSigningKey,
								  const uint64_t& inAWSSigningKeySize,
								  const std::string& inAWSRegion,
								  const std::string& inS3BucketName,
								  const std::string& inS3ObjectKey,
								  const StreamInS3ObjectDataHandlerFunctionRef& inDataHandlerFunction) {
			std::string region(inAWSRegion);
			std::string method("GET");
			
			std::string host(inS3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string urlEncodedObjectKey;
			http::URLEncode(inS3ObjectKey, false, urlEncodedObjectKey);
			std::string s3Path(urlEncodedObjectKey);
			if (!s3Path.empty() && (s3Path[0] != '/')) {
				s3Path = "/" + s3Path;
			}
			
			int redirectCount = 0;
			while (true) {
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
				
				EnumerateStringValuesFunctionClass headerParams(params);
				StreamInDataHandler dataHandler;
				StreamInResult result;
				StreamInS3Request(h_, url, method, headerParams, dataHandler, result);
				if (result.mResult != S3Result::kSuccess) {
					if (result.mResult == S3Result::kCanceled) {
						return S3Result::kCanceled;
					}
					if (result.mResult == S3Result::kAuthorizationHeaderMalformed) {
						return S3Result::kAuthorizationHeaderMalformed;
					}
					if (result.mResult == S3Result::k400BadRequest) {
						return S3Result::k400BadRequest;
					}
					if (result.mResult == S3Result::k403AccessDenied) {
						return S3Result::k403AccessDenied;
					}
					if (result.mResult == S3Result::k404EntityNotFound) {
						return S3Result::k404EntityNotFound;
					}
					if (result.mResult == S3Result::k404NoSuchBucket) {
						return S3Result::k404NoSuchBucket;
					}
					if (result.mResult == S3Result::kNetworkConnectionLost) {
						return S3Result::kNetworkConnectionLost;
					}
					if ((result.mResult == S3Result::kS3InternalError) ||
							 (result.mResult == S3Result::k500InternalServerError) ||
							 (result.mResult == S3Result::k503ServiceUnavailable)) {
						return result.mResult;
					}
					if (result.mResult == S3Result::kTimedOut) {
						return S3Result::kTimedOut;
					}
					if (result.mResult == S3Result::k301PermanentRedirect) {
						return S3Result::k301PermanentRedirect;
					}
					if (result.mResult == S3Result::k307TemporaryRedirect) {
						ProcessXMLClass pc(h_);
						pc.Process(dataHandler.mResponse);
						if (pc.mCode == "TemporaryRedirect") {
							if (pc.mEndpoint.empty()) {
								NOTIFY_ERROR(h_,
											 "StreamInS3Object: S3Result::k307TemporaryRedirect but new endpoint is empty for host:",
											 host);
								return S3Result::kError;
							}
							if (pc.mEndpoint == host) {
								NOTIFY_ERROR(h_,
											 "StreamInS3Object: S3Result::k307TemporaryRedirect but new endpoint is the same for host:",
											 host);
								return S3Result::kError;
							}
							if (++redirectCount >= 5) {
								NOTIFY_ERROR(h_, "StreamInS3Object: too many temporary redirects for host", host);
								return S3Result::kError;
							}
							host = pc.mEndpoint;
							continue;
						}
						else {
							NOTIFY_ERROR(h_,
										 "StreamInS3Object: Unparsed 307 Temporary Redirect for host:",
										 host,
										 "response:",
										 dataHandler.mResponse);
							
							return S3Result::kError;
						}
					}
					else {
						NOTIFY_ERROR(h_, "StreamInS3Object: StreamInS3Request failed for URL:", url);
						return result.mResult;
					}
				}
				else {
					//	We currently put this value when we put an s3 object, but we can't assume it's
					//	there for any given s3 object we're asked to fetch. So this checksum step is optional.
					auto it = result.mParams.find("x-amz-meta-sha256");
					if (it != result.mParams.end()) {
						std::string s3sha256hex = it->second;
						
						std::string dataSHA256;
						encoding::CalculateSHA256(dataHandler.mResponse, dataSHA256);
						if (dataSHA256.empty()) {
							NOTIFY_ERROR(h_, "StreamInS3Object: CalculateSHA256 failed.");
							return S3Result::kError;
						}
						
						std::string dataSHA256Hex;
						string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
						if (dataSHA256Hex.empty()) {
							NOTIFY_ERROR(h_, "StreamInS3Object: BinaryStringToHex failed.");
							return S3Result::kError;
						}
						
						if (s3sha256hex != dataSHA256Hex) {
							NOTIFY_ERROR(h_,
										 "StreamInS3Object: checksum mismatch for for URL:",
										 url,
										 "s3 value:",
										 s3sha256hex,
										 "local value:",
										 dataSHA256Hex);
							
							return S3Result::kChecksumMismatch;
						}
					}
					
					inDataHandlerFunction.Call(dataHandler.mResponse.size(),
											   DataBuffer(dataHandler.mResponse.data(), dataHandler.mResponse.size()),
											   true);
				}
				break;
			}
			return S3Result::kSuccess;
		}
		
	} // namespace s3
} // namespace hermit
