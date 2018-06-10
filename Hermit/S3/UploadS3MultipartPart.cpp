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
		namespace UploadS3MultipartPart_Impl {
			
			//
			std::string GetEndpoint(const S3ParamVector& params) {
				auto end = params.end();
				for (auto it = params.begin(); it != end; ++it) {
					if ((*it).first == "Endpoint") {
						return (*it).second;
					}
				}
				return "";
			}

			//
			std::string GetETag(const S3ParamVector& params) {
				auto end = params.end();
				for (auto it = params.begin(); it != end; ++it) {
					if ((*it).first == "Etag") {
						return (*it).second;
					}
				}
				return "";
			}

			//
			class Redirector {
				//
				class SendCommandCompletion : public SendS3CommandCompletion {
				public:
					//
					SendCommandCompletion(const http::HTTPSessionPtr& session,
										  const std::string& url,
										  int redirectCount,
										  const std::string& host,
										  const std::string& s3Path,
										  const std::string& awsPublicKey,
										  const std::string& awsSigningKey,
										  const std::string& awsRegion,
										  const std::string& uploadId,
										  const std::string& partNumberString,
										  const SharedBufferPtr& partData,
										  const std::string& partSizeString,
										  const std::string& dataSHA256Hex,
										  const UploadS3MultipartPartCompletionPtr& completion) :
					mSession(session),
					mURL(url),
					mRedirectCount(redirectCount),
					mHost(host),
					mS3Path(s3Path),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mUploadId(uploadId),
					mPartNumberString(partNumberString),
					mPartData(partData),
					mPartSizeString(partSizeString),
					mDataSHA256Hex(dataSHA256Hex),
					mCompletion(completion) {
					}
					
					//
					virtual void Call(const HermitPtr& h_,
									  const S3Result& result,
									  const S3ParamVector& params,
									  const DataBuffer& data) override {
						if (result == S3Result::kCanceled) {
							mCompletion->Call(h_, S3Result::kCanceled, "");
							return;
						}
						
						if (result == S3Result::k307TemporaryRedirect) {
							std::string newEndpoint(GetEndpoint(params));
							if (newEndpoint.empty()) {
								NOTIFY_ERROR(h_,
											 "S3Result::k307TemporaryRedirect but new endpoint is empty for url:",
											 mURL);
								mCompletion->Call(h_, S3Result::kError, "");
								return;
							}
							if (newEndpoint == mHost) {
								NOTIFY_ERROR(h_,
											 "S3Result::k307TemporaryRedirect but new endpoint is the same for url:",
											 mURL);
								mCompletion->Call(h_, S3Result::kError, "");
								return;
							}
							UploadS3MultipartPart(h_,
												  mSession,
												  mRedirectCount + 1,
												  newEndpoint,
												  mS3Path,
												  mAWSPublicKey,
												  mAWSSigningKey,
												  mAWSRegion,
												  mUploadId,
												  mPartNumberString,
												  mPartData,
												  mPartSizeString,
												  mDataSHA256Hex,
												  mCompletion);
							return;
						}
						if ((result == S3Result::kTimedOut) ||
							(result == S3Result::kNetworkConnectionLost) ||
							(result == S3Result::kNoNetworkConnection) ||
							(result == S3Result::k403AccessDenied) ||
							(result == S3Result::kS3InternalError) ||
							(result == S3Result::k500InternalServerError) ||
							(result == S3Result::k503ServiceUnavailable)) {
							mCompletion->Call(h_, result, "");
							return;
						}
						if (result != S3Result::kSuccess) {
							NOTIFY_ERROR(h_, "SendS3Command failed for URL:", mURL);
							mCompletion->Call(h_, S3Result::kError, "");
							return;
						}
						
						std::string eTag(GetETag(params));
						if (eTag.empty()) {
							NOTIFY_ERROR(h_, "UploadS3MultipartPart: result.mETag.empty() URL:", mURL);
							mCompletion->Call(h_, S3Result::kError, "");
							return;
						}
						mCompletion->Call(h_, S3Result::kSuccess, eTag);
					}
					
					//
					http::HTTPSessionPtr mSession;
					std::string mURL;
					int mRedirectCount;
					std::string mHost;
					std::string mS3Path;
					std::string mAWSPublicKey;
					std::string mAWSSigningKey;
					std::string mAWSRegion;
					std::string mUploadId;
					std::string mPartNumberString;
					SharedBufferPtr mPartData;
					std::string mPartSizeString;
					std::string mDataSHA256Hex;
					UploadS3MultipartPartCompletionPtr mCompletion;
				};
				
			public:
				//
				static void UploadS3MultipartPart(const HermitPtr& h_,
												  const http::HTTPSessionPtr& session,
												  int redirectCount,
												  const std::string& host,
												  const std::string& s3Path,
												  const std::string& awsPublicKey,
												  const std::string& awsSigningKey,
												  const std::string& awsRegion,
												  const std::string& uploadId,
												  const std::string& partNumberString,
												  const SharedBufferPtr& partData,
												  const std::string& partSizeString,
												  const std::string& dataSHA256Hex,
												  const UploadS3MultipartPartCompletionPtr& completion) {
					if (redirectCount > 5) {
						NOTIFY_ERROR(h_, "Too many temporary redirects for s3Path:", s3Path);
						completion->Call(h_, S3Result::kError, "");
						return;
					}
					
					time_t now;
					time(&now);
					tm globalTime;
					gmtime_r(&now, &globalTime);
					char dateBuf[2048];
					strftime(dateBuf, 2048, "%Y%m%dT%H%M%SZ", &globalTime);
					std::string dateTime(dateBuf);
					std::string date(dateTime.substr(0, 8));
					
					std::string method("PUT");
					std::string canonicalRequest(method);
					canonicalRequest += "\n";
					canonicalRequest += s3Path;
					canonicalRequest += "\n";
					canonicalRequest += "partNumber=";
					canonicalRequest += partNumberString;
					canonicalRequest += "&uploadId=";
					canonicalRequest += uploadId;
					canonicalRequest += "\n";
					canonicalRequest += "content-length:";
					canonicalRequest += partSizeString;
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
					stringToSign += awsRegion;
					stringToSign += "/s3/aws4_request";
					stringToSign += "\n";
					stringToSign += canonicalRequestSHA256Hex;
					
					std::string stringToSignSHA256;
					encoding::CalculateHMACSHA256(awsSigningKey, stringToSign, stringToSignSHA256);
					std::string stringToSignSHA256Hex;
					string::BinaryStringToHex(stringToSignSHA256, stringToSignSHA256Hex);
					
					std::string authorization("AWS4-HMAC-SHA256 Credential=");
					authorization += awsPublicKey;
					authorization += "/";
					authorization += date;
					authorization += "/";
					authorization += awsRegion;
					authorization += "/s3/aws4_request";
					authorization += ",";
					authorization += "SignedHeaders=content-length;host;x-amz-content-sha256;x-amz-date";
					authorization += ",";
					authorization += "Signature=";
					authorization += stringToSignSHA256Hex;
					
					S3ParamVector params;
					params.push_back(std::make_pair("Content-Length", partSizeString));
					params.push_back(std::make_pair("x-amz-date", dateTime));
					params.push_back(std::make_pair("x-amz-content-sha256", dataSHA256Hex));
					params.push_back(std::make_pair("Authorization", authorization));
					params.push_back(std::make_pair("Expect", "100-continue"));
					
					std::string url("https://");
					url += host;
					url += s3Path;
					url += "?partNumber=";
					url += partNumberString;
					url += "&uploadId=";
					url += uploadId;
					
					auto commandCompletion = std::make_shared<SendCommandCompletion>(session,
																					 url,
																					 redirectCount,
																					 host,
																					 s3Path,
																					 awsPublicKey,
																					 awsSigningKey,
																					 awsRegion,
																					 uploadId,
																					 partNumberString,
																					 partData,
																					 partSizeString,
																					 dataSHA256Hex,
																					 completion);
					SendS3CommandWithData(h_, session, url, method, params, partData, commandCompletion);
				}
			};

		} // namespace UploadS3MultipartPart_Impl
		using namespace UploadS3MultipartPart_Impl;
		
		//
		void UploadS3MultipartPart(const HermitPtr& h_,
								   const http::HTTPSessionPtr& session,
								   const std::string& awsPublicKey,
								   const std::string& awsSigningKey,
								   const std::string& awsRegion,
								   const std::string& s3BucketName,
								   const std::string& s3ObjectKey,
								   const std::string& uploadId,
								   const int32_t& partNumber,
								   const SharedBufferPtr& partData,
								   const UploadS3MultipartPartCompletionPtr& completion) {
			std::string host(s3BucketName);
			host += ".s3.amazonaws.com";
			std::string s3Path(s3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			std::string partNumberString;
			string::UInt32ToString(partNumber, partNumberString);
			if (partNumberString.empty()) {
				NOTIFY_ERROR(h_, "UploadS3MultipartPart: UInt32ToString failed for partNumber:", partNumber);
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			std::string partSizeString;
			string::UInt64ToString(h_, partData->Size(), partSizeString);
			if (partSizeString.empty()) {
				NOTIFY_ERROR(h_, "UploadS3MultipartPart: UInt64ToString failed for partSize:", partData->Size());
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256;
			encoding::CalculateSHA256(std::string(partData->Data(), partData->Size()), dataSHA256);
			if (dataSHA256.empty()) {
				NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: CalculateSHA256 failed for partNumber:", partNumber);
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256Hex;
			string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
			if (dataSHA256Hex.empty()) {
				NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: BinaryStringToHex failed for partNumber:", partNumber);
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			Redirector::UploadS3MultipartPart(h_,
											  session,
											  0,
											  host,
											  s3Path,
											  awsPublicKey,
											  awsSigningKey,
											  awsRegion,
											  uploadId,
											  partNumberString,
											  partData,
											  partSizeString,
											  dataSHA256Hex,
											  completion);
		}
		
	} // namespace s3
} // namespace hermit
