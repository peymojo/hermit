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

#include <string>
#include <vector>
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/HTTP/URLEncode.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "S3GetListObjectsXML.h"

namespace hermit {
	namespace s3 {
		
		//
		namespace {

			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			class ParamCallback : public EnumerateStringValuesCallback {
			public:
				//
				bool Function(const std::string& inName, const std::string& inValue) {
					if (inName == "Endpoint") {
						mNewEndpoint = inValue;
					}
					return true;
				}
				
				//
				std::string mNewEndpoint;
			};
			
			//
			class SendCommandCallback : public SendS3CommandCallback {
			public:
				//
				SendCommandCallback() : mStatus(S3Result::kUnknown) {
				}
				
				//
				bool Function(const S3Result& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const DataBuffer& inData) {
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess) {
						mResponseData = std::string(inData.first, inData.second);
					}
					else if (inStatus == S3Result::k307TemporaryRedirect) {
						ParamCallback callback;
						inParamFunction.Call(callback);
						mNewEndpoint = callback.mNewEndpoint;
					}
					return true;
				}
				
				//
				S3Result mStatus;
				std::string mResponseData;
				std::string mNewEndpoint;
			};
			
		} // private namespace
		
		//
		void S3GetListObjectsXML(const HermitPtr& h_,
								 const std::string& inAWSPublicKey,
								 const std::string& inAWSSigningKey,
								 const std::string& inAWSRegion,
								 const std::string& inBucketName,
								 const std::string& inObjectPrefix,
								 const std::string& inMarker,
								 const S3GetListObjectsXMLCallbackRef& inCallback) {
			
			std::string marker;
			http::URLEncode(inMarker, true, marker);
			
			std::string objectPrefix;
			http::URLEncode(inObjectPrefix, true, objectPrefix);
			
			std::string region(inAWSRegion);
			std::string method("GET");
			
			std::string host(inBucketName);
			host += ".s3.amazonaws.com";
			
			std::string s3Path("/");
			
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
				canonicalRequest += "prefix=";
				canonicalRequest += objectPrefix;
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
				std::string canonicalRequestSHA256HexString;
				string::BinaryStringToHex(canonicalRequestSHA256, canonicalRequestSHA256HexString);
				
				std::string stringToSign("AWS4-HMAC-SHA256");
				stringToSign += "\n";
				stringToSign += dateTime;
				stringToSign += "\n";
				stringToSign += date;
				stringToSign += "/";
				stringToSign += region;
				stringToSign += "/s3/aws4_request";
				stringToSign += "\n";
				stringToSign += canonicalRequestSHA256HexString;
				
				std::string stringToSignSHA256;
				encoding::CalculateHMACSHA256(inAWSSigningKey, stringToSign, stringToSignSHA256);
				std::string stringToSignSHA256HexString;
				string::BinaryStringToHex(stringToSignSHA256, stringToSignSHA256HexString);
				
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
				authorization += stringToSignSHA256HexString;
				
				StringPairVector params;
				params.push_back(StringPair("x-amz-date", dateTime));
				params.push_back(StringPair("x-amz-content-sha256", contentSHA256));
				params.push_back(StringPair("Authorization", authorization));
				
				std::string url("https://");
				url += host;
				
				if (!marker.empty()) {
					url += "?marker=";
					url += marker;
					if (!objectPrefix.empty()) {
						url += "&prefix=";
						url += objectPrefix;
					}
				}
				else if (!objectPrefix.empty()) {
					url += "?prefix=";
					url += objectPrefix;
				}
				
				EnumerateStringValuesFunctionClass headerParams(params);
				SendCommandCallback result;
				SendS3Command(h_, url, method, headerParams, result);
				if (result.mStatus == S3Result::kCanceled) {
					inCallback.Call(h_, S3Result::kCanceled, "");
					return;
				}
				if (result.mStatus == S3Result::k307TemporaryRedirect) {
					if (result.mNewEndpoint.empty()) {
						NOTIFY_ERROR(h_,
									 "S3GetListObjectsXML: S3Result::k307TemporaryRedirect but new endpoint is empty for url:",
									 url);
						inCallback.Call(h_, S3Result::kError, "");
						return;
					}
					if (result.mNewEndpoint == host) {
						NOTIFY_ERROR(h_,
									 "S3GetListObjectsXML: S3Result::k307TemporaryRedirect but new endpoint is the same for url:",
									 url);
						inCallback.Call(h_, S3Result::kError, "");
						return;
					}
					if (++redirectCount >= 5) {
						NOTIFY_ERROR(h_, "S3GetListObjectsXML: too many temporary redirects for url:", url);
						inCallback.Call(h_, S3Result::kError, "");
						return;
					}
					host = result.mNewEndpoint;
					continue;
				}
				if (result.mStatus == S3Result::kTimedOut) {
					inCallback.Call(h_, S3Result::kTimedOut, "");
					return;
				}
				if (result.mStatus == S3Result::k403AccessDenied) {
					inCallback.Call(h_, S3Result::k403AccessDenied, "");
					return;
				}
				if ((result.mStatus == S3Result::kS3InternalError) ||
					(result.mStatus == S3Result::k500InternalServerError) ||
					(result.mStatus == S3Result::k503ServiceUnavailable)) {
					inCallback.Call(h_, result.mStatus, "");
					return;
				}
				if (result.mStatus != S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "S3GetListObjectsXML: SendS3Command failed for URL:", url);
					inCallback.Call(h_, S3Result::kError, "");
					return;
				}
				inCallback.Call(h_, S3Result::kSuccess, result.mResponseData);
				break;
			}
		}
		
	} // namespace s3
} // namespace hermit
