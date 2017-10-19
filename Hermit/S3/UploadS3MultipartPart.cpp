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
#include "Hermit/String/HexStringToBinary.h"
#include "Hermit/String/UInt32ToString.h"
#include "Hermit/String/UInt64ToString.h"
#include "SendS3CommandWithData.h"
#include "UploadS3MultipartPart.h"

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
			class ETagParamCallback : public EnumerateStringValuesCallback {
			public:
				//
				bool Function(const std::string& inName, const std::string& inValue) {
					if (inName == "Etag") {
						mETag = inValue;
					}
					return true;
				}
				
				//
				std::string mETag;
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
						ETagParamCallback callback;
						inParamFunction.Call(callback);
						mETag = callback.mETag;
						
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
				std::string mETag;
			};
			
		} // private namespace
		
		//
		void UploadS3MultipartPart(const HermitPtr& h_,
								   const std::string& inAWSPublicKey,
								   const std::string& inAWSSigningKey,
								   const std::string& inAWSRegion,
								   const std::string& inS3BucketName,
								   const std::string& inS3ObjectKey,
								   const std::string& inUploadID,
								   const int32_t& inPartNumber,
								   const DataBuffer& inPartData,
								   const UploadS3MultipartPartCallbackRef& inCallback) {
			std::string region(inAWSRegion);
			std::string method("PUT");
			
			std::string host(inS3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string s3Path(inS3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			std::string partNumber;
			string::UInt32ToString(inPartNumber, partNumber);
			if (partNumber.empty()) {
				NOTIFY_ERROR(h_, "UploadS3MultipartPart: UInt32ToString failed for partNumber:", inPartNumber);
				inCallback.Call(S3Result::kError, "");
				return;
			}
			
			std::string partSize;
			string::UInt64ToString(inPartData.second, partSize);
			if (partSize.empty()) {
				NOTIFY_ERROR(h_, "UploadS3MultipartPart: UInt64ToString failed for partSize:", inPartData.second);
				inCallback.Call(S3Result::kError, "");
				return;
			}
			
			//	encoding::CalculateMD5CallbackClassT<std::string> md5Callback;
			//	encoding::CalculateMD5(inPartData, inPartDataSize, md5Callback);
			//	if (md5Callback.mStatus == encoding::kCalculateMD5Status_Canceled)
			//	{
			//		inCallback.Call(S3Result::kCanceled, "");
			//		return;
			//	}
			//	if (md5Callback.mStatus != encoding::kCalculateMD5Status_Success)
			//	{
			//		LogSInt32("GetS3ObjectWithVersion: CalculateMD5 failed for partNumber: ", inPartNumber);
			//		inCallback.Call(S3Result::kError, "");
			//		return;
			//	}
			//
			//	BinaryCallbackClass md5Binary;
			//	string::HexStringToBinary(md5Callback.mMD5Hex, md5Binary);
			//	if (md5Binary.mValue.empty())
			//	{
			//		LogSInt32("GetS3ObjectWithVersion: HexStringToBinary failed for partNumber: ", inPartNumber);
			//		inCallback.Call(S3Result::kError, "");
			//		return;
			//	}
			//
			//	StringCallbackClass md5Base64;
			//	encoding::BinaryToBase64(md5Binary.mValue.data(), md5Binary.mValue.size(), md5Base64);
			//	if (md5Base64.mValue.empty())
			//	{
			//		LogSInt32("GetS3ObjectWithVersion: BinaryToBase64 failed for partNumber: ", inPartNumber);
			//		inCallback.Call(S3Result::kError, "");
			//		return;
			//	}
			
			std::string dataSHA256;
			encoding::CalculateSHA256(std::string(inPartData.first, inPartData.second), dataSHA256);
			if (dataSHA256.empty()) {
				NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: CalculateSHA256 failed for partNumber:", inPartNumber);
				inCallback.Call(S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256Hex;
			string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
			if (dataSHA256Hex.empty()) {
				NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: BinaryStringToHex failed for partNumber:", inPartNumber);
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
				
				
				
				
				
				//		std::string contentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
				
				std::string canonicalRequest(method);
				canonicalRequest += "\n";
				canonicalRequest += s3Path;
				canonicalRequest += "\n";
				canonicalRequest += "partNumber=";
				canonicalRequest += partNumber;
				canonicalRequest += "&uploadId=";
				canonicalRequest += inUploadID;
				canonicalRequest += "\n";
				canonicalRequest += "content-length:";
				canonicalRequest += partSize;
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
				canonicalRequest += "\n";
				canonicalRequest += "content-length;host;x-amz-content-sha256;x-amz-date";
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
				authorization += "SignedHeaders=content-length;host;x-amz-content-sha256;x-amz-date";
				authorization += ",";
				authorization += "Signature=";
				authorization += stringToSignSHA256Hex;
				
				StringPairVector params;
				params.push_back(StringPair("Content-Length", partSize));
				params.push_back(StringPair("x-amz-date", dateTime));
				params.push_back(StringPair("x-amz-content-sha256", dataSHA256Hex));
				params.push_back(StringPair("Authorization", authorization));
				params.push_back(StringPair("Expect", "100-continue"));
				
				std::string url("https://");
				url += host;
				url += s3Path;
				url += "?partNumber=";
				url += partNumber;
				url += "&uploadId=";
				url += inUploadID;
				
				EnumerateStringValuesFunctionClass headerParams(params);
				SendCommandCallback result;
				SendS3CommandWithData(h_, url, method, headerParams, inPartData, result);
				if (result.mStatus == S3Result::kCanceled) {
					inCallback.Call(S3Result::kCanceled, "");
					return;
				}
				if (result.mStatus == S3Result::k307TemporaryRedirect) {
					if (result.mNewEndpoint.empty()) {
						NOTIFY_ERROR(h_,
									 "UploadS3MultipartPart: S3Result::k307TemporaryRedirect but new endpoint is empty for url:",
									 url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (result.mNewEndpoint == host) {
						NOTIFY_ERROR(h_,
									 "UploadS3MultipartPart: S3Result::k307TemporaryRedirect but new endpoint is the same for url:",
									 url);
						inCallback.Call(S3Result::kError, "");
						return;
					}
					if (++redirectCount >= 5) {
						NOTIFY_ERROR(h_, "UploadS3MultipartPart: too many temporary redirects for url:", url);
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
					inCallback.Call(S3Result::k403AccessDenied, "");
					return;
				}
				if ((result.mStatus == S3Result::kS3InternalError) ||
					(result.mStatus == S3Result::k500InternalServerError) ||
					(result.mStatus == S3Result::k503ServiceUnavailable)) {
					inCallback.Call(result.mStatus, "");
					return;
				}
				if (result.mStatus != S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "UploadS3MultipartPart: SendS3Command failed for URL:", url);
					inCallback.Call(S3Result::kError, "");
					return;
				}
				
				if (result.mETag.empty()) {
					NOTIFY_ERROR(h_, "UploadS3MultipartPart: result.mETag.empty() URL:", url);
					inCallback.Call(S3Result::kError, "");
					return;
				}
				
				inCallback.Call(S3Result::kSuccess, result.mETag);
				break;
			}
		}
		
	} // namespace s3
} // namespace hermit
