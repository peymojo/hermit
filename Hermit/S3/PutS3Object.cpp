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
#include "Hermit/HTTP/SendHTTPRequestWithBody.h"
#include "Hermit/HTTP/URLEncode.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/String/UInt64ToString.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"
#include "PutS3Object.h"

namespace hermit {
	namespace s3 {
		
		namespace {

			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			class EndpointParamCallback : public EnumerateStringValuesCallback {
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
			class VersionParamCallback : public EnumerateStringValuesCallback {
			public:
				//
				bool Function(const std::string& inName, const std::string& inValue) {
					if ((inName == "X-Amz-Version-Id") || (inName == "x-amz-version-id")) {
						mVersion = inValue;
					}
					return true;
				}
				
				//
				std::string mVersion;
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
						VersionParamCallback callback;
						inParamFunction.Call(callback);
						mVersion = callback.mVersion;
						
						mResponseData = std::string(inData.first, inData.second);
					}
					else if (inStatus == S3Result::k307TemporaryRedirect) {
						EndpointParamCallback callback;
						inParamFunction.Call(callback);
						mNewEndpoint = callback.mNewEndpoint;
					}
					return true;
				}
				
				//
				S3Result mStatus;
				std::string mResponseData;
				std::string mNewEndpoint;
				std::string mVersion;
			};
			
		} // private namespace
		
		//
		//
		void PutS3Object(const HermitPtr& h_,
						 const std::string& inAWSPublicKey,
						 const std::string& inAWSSigningKey,
						 const std::string& inAWSRegion,
						 const std::string& inS3BucketName,
						 const std::string& inS3ObjectKey,
						 const DataBuffer& inData,
						 const bool& inUseReducedRedundancyStorage,
						 const PutS3ObjectCallbackRef& inCallback) {
			std::string region(inAWSRegion);
			std::string method("PUT");
			
			std::string host(inS3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string s3Path(inS3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			std::string dataSize;
			string::UInt64ToString(inData.second, dataSize);
			if (dataSize.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: UInt64ToString failed for inDataSize:", inData.second);
				inCallback.Call(S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256;
			encoding::CalculateSHA256(std::string(inData.first, inData.second), dataSHA256);
			if (dataSHA256.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: CalculateSHA256 failed.");
				inCallback.Call(S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256Hex;
			string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
			if (dataSHA256Hex.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: BinaryStringToHex failed.");
				inCallback.Call(S3Result::kError, "");
				return;
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
				
				std::string canonicalRequest(method);
				canonicalRequest += "\n";
				canonicalRequest += s3Path;
				canonicalRequest += "\n";
				canonicalRequest += "\n";
				canonicalRequest += "content-length:";
				canonicalRequest += dataSize;
				canonicalRequest += "\n";
				canonicalRequest += "host:";
				canonicalRequest += host;
				canonicalRequest += "\n";
				canonicalRequest += "x-amz-content-sha256:";
				canonicalRequest += dataSHA256Hex;
				canonicalRequest += "\n";
				canonicalRequest += "x-amz-date:";
				canonicalRequest += dateTime;
				canonicalRequest += "\n";
				canonicalRequest += "x-amz-meta-sha256:";
				canonicalRequest += dataSHA256Hex;
				canonicalRequest += "\n";
				canonicalRequest += "\n";
				canonicalRequest += "content-length;host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
				canonicalRequest += "\n";
				canonicalRequest += dataSHA256Hex;
				
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
				
				std::string stringToSignHMACSHA256;
				encoding::CalculateHMACSHA256(inAWSSigningKey, stringToSign, stringToSignHMACSHA256);
				std::string stringToSignHMACSHA256Hex;
				string::BinaryStringToHex(stringToSignHMACSHA256, stringToSignHMACSHA256Hex);
				
				std::string authorization("AWS4-HMAC-SHA256 Credential=");
				authorization += inAWSPublicKey;
				authorization += "/";
				authorization += date;
				authorization += "/";
				authorization += region;
				authorization += "/s3/aws4_request";
				authorization += ",";
				authorization += "SignedHeaders=content-length;host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
				authorization += ",";
				authorization += "Signature=";
				authorization += stringToSignHMACSHA256Hex;
				
				StringPairVector params;
				params.push_back(StringPair("Content-Length", dataSize));
				params.push_back(StringPair("x-amz-date", dateTime));
				params.push_back(StringPair("x-amz-content-sha256", dataSHA256Hex));
				params.push_back(StringPair("x-amz-meta-sha256", dataSHA256Hex));
				params.push_back(StringPair("Authorization", authorization));
				params.push_back(StringPair("Expect", "100-continue"));
				
				std::string url("https://");
				url += host;
				url += s3Path;
				
				EnumerateStringValuesFunctionClass headerParams(params);
				SendCommandCallback result;
				SendS3CommandWithData(h_,
									  url,
									  method,
									  headerParams,
									  inData,
									  result);
				
				if (result.mStatus == S3Result::kCanceled) {
					inCallback.Call(S3Result::kCanceled, "");
					return;
				}
				if (result.mStatus == S3Result::k307TemporaryRedirect) {
					if (result.mNewEndpoint.empty()) {
						NOTIFY_ERROR(h_, "PutS3Object: S3Result::k307TemporaryRedirect but new endpoint is empty for url:", url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (result.mNewEndpoint == host) {
						NOTIFY_ERROR(h_, "PutS3Object: S3Result::k307TemporaryRedirect but new endpoint is the same for url:", url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (++redirectCount >= 5) {
						NOTIFY_ERROR(h_, "PutS3Object: too many temporary redirects for url:", url);
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
				if ((result.mStatus == S3Result::kS3InternalError) ||
					(result.mStatus == S3Result::k500InternalServerError) ||
					(result.mStatus == S3Result::k503ServiceUnavailable)) {
					inCallback.Call(result.mStatus, "");
					return;
				}
				if (result.mStatus == S3Result::k403AccessDenied) {
					NOTIFY_ERROR(h_, "PutS3Object: S3Result::k403AccessDenied for URL:", url);
					inCallback.Call(S3Result::k403AccessDenied, "");
					return;
				}
				if (result.mStatus != S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "PutS3Object: SendS3Command failed for URL:", url);
					inCallback.Call(S3Result::kError, "");
					return;
				}
				
				inCallback.Call(S3Result::kSuccess, result.mVersion);
				break;
			}
		}
		
	} // namespace s3
} // namespace hermit

