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

#include <sstream>
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "CompleteS3MultipartUpload.h"
#include "SendS3CommandWithData.h"

namespace hermit {
	namespace s3 {
		namespace CompleteS3MultipartUpload_Impl {
			
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
			class Redirector {
				//
				class SendCommandCompletion : public SendS3CommandCompletion {
				public:
					//
					SendCommandCompletion(const std::string& url,
										  int redirectCount,
										  const std::string& host,
										  const std::string& s3Path,
										  const std::string& payload,
										  const std::string& contentSHA256Hex,
										  const std::string& awsPublicKey,
										  const std::string& awsSigningKey,
										  const std::string& awsRegion,
										  const std::string& uploadId,
										  const S3CompletionBlockPtr& completion) :
					mURL(url),
					mRedirectCount(redirectCount),
					mHost(host),
					mS3Path(s3Path),
					mPayload(payload),
					mContentSHA256Hex(contentSHA256Hex),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mUploadId(uploadId),
					mCompletion(completion) {
					}
					
					//
					virtual void Call(const HermitPtr& h_,
									  const S3Result& result,
									  const S3ParamVector& params,
									  const DataBuffer& data) override {
						if (result == S3Result::kCanceled) {
							mCompletion->Call(h_, S3Result::kCanceled);
							return;
						}
						
						if (result == S3Result::k307TemporaryRedirect) {
							std::string newEndpoint(GetEndpoint(params));
							if (newEndpoint.empty()) {
								NOTIFY_ERROR(h_,
											 "S3Result::k307TemporaryRedirect but new endpoint is empty for url:",
											 mURL);
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
							if (newEndpoint == mHost) {
								NOTIFY_ERROR(h_,
											 "S3Result::k307TemporaryRedirect but new endpoint is the same for url:",
											 mURL);
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
							CompleteS3MultipartUpload(h_,
													  mRedirectCount + 1,
													  newEndpoint,
													  mS3Path,
													  mPayload,
													  mContentSHA256Hex,
													  mAWSPublicKey,
													  mAWSSigningKey,
													  mAWSRegion,
													  mUploadId,
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
							mCompletion->Call(h_, result);
							return;
						}
						if (result != S3Result::kSuccess) {
							NOTIFY_ERROR(h_, "SendS3Command failed for URL:", mURL);
							mCompletion->Call(h_, S3Result::kError);
							return;
						}
						mCompletion->Call(h_, S3Result::kSuccess);
					}
					
					//
					std::string mURL;
					int mRedirectCount;
					std::string mHost;
					std::string mS3Path;
					std::string mPayload;
					std::string mContentSHA256Hex;
					std::string mAWSPublicKey;
					std::string mAWSSigningKey;
					std::string mAWSRegion;
					std::string mUploadId;
					S3CompletionBlockPtr mCompletion;
				};

			public:
				//
				static void CompleteS3MultipartUpload(const HermitPtr& h_,
													  int redirectCount,
													  const std::string& host,
													  const std::string& s3Path,
													  const std::string& payload,
													  const std::string& contentSHA256Hex,
													  const std::string& awsPublicKey,
													  const std::string& awsSigningKey,
													  const std::string& awsRegion,
													  const std::string& uploadId,
													  const S3CompletionBlockPtr& completion) {
					if (redirectCount > 5) {
						NOTIFY_ERROR(h_, "Too many temporary redirects for s3Path:", s3Path);
						completion->Call(h_, S3Result::kError);
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
					
					std::string method("POST");
					std::string canonicalRequest(method);
					canonicalRequest += "\n";
					canonicalRequest += s3Path;
					canonicalRequest += "\n";
					canonicalRequest += "uploadId=";
					canonicalRequest += uploadId;
					canonicalRequest += "\n";
					canonicalRequest += "host:";
					canonicalRequest += host;
					canonicalRequest += "\n";
					canonicalRequest += "x-amz-content-sha256:";
					canonicalRequest += contentSHA256Hex;
					canonicalRequest += "\n";
					canonicalRequest += "x-amz-date:";
					canonicalRequest += dateTime;
					canonicalRequest += "\n";
					canonicalRequest += "\n";
					canonicalRequest += "host;x-amz-content-sha256;x-amz-date";
					canonicalRequest += "\n";
					canonicalRequest += contentSHA256Hex;
					
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
					std::string requestStringSHA256Hex;
					string::BinaryStringToHex(stringToSignSHA256, requestStringSHA256Hex);
					
					std::string authorization("AWS4-HMAC-SHA256 Credential=");
					authorization += awsPublicKey;
					authorization += "/";
					authorization += date;
					authorization += "/";
					authorization += awsRegion;
					authorization += "/s3/aws4_request";
					authorization += ",";
					authorization += "SignedHeaders=host;x-amz-content-sha256;x-amz-date";
					authorization += ",";
					authorization += "Signature=";
					authorization += requestStringSHA256Hex;
					
					S3ParamVector params;
					params.push_back(std::make_pair("x-amz-date", dateTime));
					params.push_back(std::make_pair("x-amz-content-sha256", contentSHA256Hex));
					params.push_back(std::make_pair("Authorization", authorization));
					params.push_back(std::make_pair("Content-Type", "text/plain"));
					
					std::string url("https://");
					url += host;
					url += s3Path;
					url += "?uploadId=";
					url += uploadId;
					
					auto commandCompletion = std::make_shared<SendCommandCompletion>(url,
																					 redirectCount,
																					 host,
																					 s3Path,
																					 payload,
																					 contentSHA256Hex,
																					 awsPublicKey,
																					 awsSigningKey,
																					 awsRegion,
																					 uploadId,
																					 completion);
					SendS3CommandWithData(h_,
										  url,
										  method,
										  params,
										  std::make_shared<SharedBuffer>(payload),
										  commandCompletion);
				}
			};
			
		} // namespace CompleteS3MultipartUpload_Impl
		using namespace CompleteS3MultipartUpload_Impl;
		
		//
		void CompleteS3MultipartUpload(const HermitPtr& h_,
									   const std::string& awsPublicKey,
									   const std::string& awsSigningKey,
									   const std::string& awsRegion,
									   const std::string& s3BucketName,
									   const std::string& s3ObjectKey,
									   const std::string& uploadId,
									   const PartVector& parts,
									   const S3CompletionBlockPtr& completion) {
			std::ostringstream stream;
			auto end = parts.end();
			for (auto it = parts.begin(); it != end; ++it) {
				stream << "<Part><PartNumber>"
				<< (*it).first
				<< "</PartNumber><ETag>"
				<< (*it).second
				<< "</ETag></Part>"
				<< "\n";
			}
			std::string payload("<CompleteMultipartUpload>\n");
			payload += stream.str();
			payload += "</CompleteMultipartUpload>";
			
			std::string contentSHA256;
			encoding::CalculateSHA256(payload, contentSHA256);
			if (contentSHA256.empty()) {
				NOTIFY_ERROR(h_, "CompleteS3MultipartUpload: CalculateSHA256 failed for payload.");
				completion->Call(h_, S3Result::kError);
				return;
			}
			
			std::string contentSHA256Hex;
			string::BinaryStringToHex(contentSHA256, contentSHA256Hex);
			if (contentSHA256Hex.empty()) {
				NOTIFY_ERROR(h_, "CompleteS3MultipartUpload: CalculateSHA256 failed for payload.");
				completion->Call(h_, S3Result::kError);
				return;
			}
			
			std::string host(s3BucketName);
			host += ".s3.amazonaws.com";
			std::string s3Path(s3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			Redirector::CompleteS3MultipartUpload(h_,
												  0,
												  host,
												  s3Path,
												  payload,
												  contentSHA256Hex,
												  awsPublicKey,
												  awsSigningKey,
												  awsRegion,
												  uploadId,
												  completion);
		}
		
	} // namespace s3
} // namespace hermit
