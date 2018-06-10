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
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/String/UInt64ToString.h"
#include "PutS3Object.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"

namespace hermit {
	namespace s3 {
		namespace PutS3Object_Impl {

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
			std::string GetVersion(const S3ParamVector& params) {
				auto end = params.end();
				for (auto it = params.begin(); it != end; ++it) {
					if (((*it).first == "X-Amz-Version-Id") || ((*it).first == "x-amz-version-id")) {
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
										  const SharedBufferPtr& data,
										  const std::string& dataSizeString,
										  const std::string& dataSHA256Hex,
										  const std::string& awsPublicKey,
										  const std::string& awsSigningKey,
										  const std::string& awsRegion,
										  const PutS3ObjectCompletionPtr& completion) :
					mSession(session),
					mURL(url),
					mRedirectCount(redirectCount),
					mHost(host),
					mS3Path(s3Path),
					mData(data),
					mDataSizeString(dataSizeString),
					mDataSHA256Hex(dataSHA256Hex),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mCompletion(completion) {
					}
					
					//
					virtual void Call(const HermitPtr& h_,
									  const S3Result& result,
									  const S3ParamVector& params,
									  const DataBuffer& responseData) override {
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
							PutS3Object(h_,
										mSession,
										mRedirectCount + 1,
										newEndpoint,
										mS3Path,
										mData,
										mDataSizeString,
										mDataSHA256Hex,
										mAWSPublicKey,
										mAWSSigningKey,
										mAWSRegion,
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
						
						std::string version = GetVersion(params);
						mCompletion->Call(h_, S3Result::kSuccess, version);
					}
					
					//
					http::HTTPSessionPtr mSession;
					std::string mURL;
					int mRedirectCount;
					std::string mHost;
					std::string mS3Path;
					SharedBufferPtr mData;
					std::string mDataSizeString;
					std::string mDataSHA256Hex;
					std::string mAWSPublicKey;
					std::string mAWSSigningKey;
					std::string mAWSRegion;
					PutS3ObjectCompletionPtr mCompletion;
				};
				
			public:
				//
				static void PutS3Object(const HermitPtr& h_,
										const http::HTTPSessionPtr& session,
										int redirectCount,
										const std::string& host,
										const std::string& s3Path,
										const SharedBufferPtr& data,
										const std::string& dataSizeString,
										const std::string& dataSHA256Hex,
										const std::string& awsPublicKey,
										const std::string& awsSigningKey,
										const std::string& awsRegion,
										const PutS3ObjectCompletionPtr& completion) {
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
					canonicalRequest += "\n";
					canonicalRequest += "content-length:";
					canonicalRequest += dataSizeString;
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
					stringToSign += awsRegion;
					stringToSign += "/s3/aws4_request";
					stringToSign += "\n";
					stringToSign += canonicalRequestSHA256Hex;
					
					std::string stringToSignHMACSHA256;
					encoding::CalculateHMACSHA256(awsSigningKey, stringToSign, stringToSignHMACSHA256);
					std::string stringToSignHMACSHA256Hex;
					string::BinaryStringToHex(stringToSignHMACSHA256, stringToSignHMACSHA256Hex);
					
					std::string authorization("AWS4-HMAC-SHA256 Credential=");
					authorization += awsPublicKey;
					authorization += "/";
					authorization += date;
					authorization += "/";
					authorization += awsRegion;
					authorization += "/s3/aws4_request";
					authorization += ",";
					authorization += "SignedHeaders=content-length;host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
					authorization += ",";
					authorization += "Signature=";
					authorization += stringToSignHMACSHA256Hex;
					
					S3ParamVector params;
					params.push_back(std::make_pair("Content-Length", dataSizeString));
					params.push_back(std::make_pair("x-amz-date", dateTime));
					params.push_back(std::make_pair("x-amz-content-sha256", dataSHA256Hex));
					params.push_back(std::make_pair("x-amz-meta-sha256", dataSHA256Hex));
					params.push_back(std::make_pair("Authorization", authorization));
					params.push_back(std::make_pair("Expect", "100-continue"));
					
					std::string url("https://");
					url += host;
					url += s3Path;
					
					auto commandCompletion = std::make_shared<SendCommandCompletion>(session,
																					 url,
																					 redirectCount,
																					 host,
																					 s3Path,
																					 data,
																					 dataSizeString,
																					 dataSHA256Hex,
																					 awsPublicKey,
																					 awsSigningKey,
																					 awsRegion,
																					 completion);					
					SendS3CommandWithData(h_,
										  session,
										  url,
										  method,
										  params,
										  data,
										  commandCompletion);
				}
			};

		} // namespace PutS3Object_Impl
		using namespace PutS3Object_Impl;
		
		//
		void PutS3Object(const HermitPtr& h_,
						 const http::HTTPSessionPtr& session,
						 const std::string& awsPublicKey,
						 const std::string& awsSigningKey,
						 const std::string& awsRegion,
						 const std::string& s3BucketName,
						 const std::string& s3ObjectKey,
						 const SharedBufferPtr& data,
						 const bool& useReducedRedundancyStorage,
						 const PutS3ObjectCompletionPtr& completion) {
			std::string host(s3BucketName);
			host += ".s3.amazonaws.com";
			std::string s3Path(s3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			std::string dataSizeString;
			string::UInt64ToString(h_, data->Size(), dataSizeString);
			if (dataSizeString.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: UInt64ToString failed for data->Size():", data->Size());
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256;
			encoding::CalculateSHA256(std::string(data->Data(), data->Size()), dataSHA256);
			if (dataSHA256.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: CalculateSHA256 failed.");
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			std::string dataSHA256Hex;
			string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
			if (dataSHA256Hex.empty()) {
				NOTIFY_ERROR(h_, "PutS3Object: BinaryStringToHex failed.");
				completion->Call(h_, S3Result::kError, "");
				return;
			}
			
			Redirector::PutS3Object(h_,
									session,
									0,
									host,
									s3Path,
									data,
									dataSizeString,
									dataSHA256Hex,
									awsPublicKey,
									awsSigningKey,
									awsRegion,
									completion);
		}
		
	} // namespace s3
} // namespace hermit

