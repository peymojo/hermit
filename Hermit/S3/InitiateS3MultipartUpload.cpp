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
		namespace InitiateS3MultipartUpload_Impl {
			
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
			class ProcessXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kInitiateMultipartUploadResult,
					kUploadId,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessXMLClass(const HermitPtr& h_) : mH_(h_), mParseState(ParseState::kNew) {
				}
				
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == ParseState::kNew) {
						if (inStartTag == "InitiateMultipartUploadResult") {
							PushState(ParseState::kInitiateMultipartUploadResult);
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kInitiateMultipartUploadResult) {
						if (inStartTag == "UploadId") {
							PushState(ParseState::kUploadId);
						}
						else {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else {
						PushState(ParseState::kIgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == ParseState::kUploadId) {
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
				std::string mUploadId;
			};

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
										  const std::string& dataSHA256Hex,
										  const std::string& awsPublicKey,
										  const std::string& awsSigningKey,
										  const std::string& awsRegion,
										  const InitiateS3MultipartUploadCompletionPtr& completion) :
					mURL(url),
					mRedirectCount(redirectCount),
					mHost(host),
					mS3Path(s3Path),
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
							InitiateS3MultipartUpload(h_,
													  mRedirectCount + 1,
													  newEndpoint,
													  mS3Path,
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
						if ((responseData.first == nullptr) || (responseData.second == 0)) {
							NOTIFY_ERROR(h_, "No response data? URL:", mURL);
							mCompletion->Call(h_, S3Result::kError, "");
							return;
						}
						
						ProcessXMLClass xmlClass(h_);
						xmlClass.Process(std::string(responseData.first, responseData.second));
						if (xmlClass.mUploadId.empty()) {
							NOTIFY_ERROR(h_, "xmlClass.mUploadId.empty() URL:", mURL);
							mCompletion->Call(h_, S3Result::kError, "");
							return;
						}
						mCompletion->Call(h_, S3Result::kSuccess, xmlClass.mUploadId);
					}
					
					//
					std::string mURL;
					int mRedirectCount;
					std::string mHost;
					std::string mS3Path;
					std::string mDataSHA256Hex;
					std::string mAWSPublicKey;
					std::string mAWSSigningKey;
					std::string mAWSRegion;
					InitiateS3MultipartUploadCompletionPtr mCompletion;
				};
				
			public:
				//
				static void InitiateS3MultipartUpload(const HermitPtr& h_,
													  int redirectCount,
													  const std::string& host,
													  const std::string& s3Path,
													  const std::string& dataSHA256Hex,
													  const std::string& awsPublicKey,
													  const std::string& awsSigningKey,
													  const std::string& awsRegion,
													  const InitiateS3MultipartUploadCompletionPtr& completion) {
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
					
					std::string contentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
					
					std::string method("POST");
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
					canonicalRequest += dataSHA256Hex;
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
					authorization += "SignedHeaders=host;x-amz-content-sha256;x-amz-date;x-amz-meta-sha256";
					authorization += ",";
					authorization += "Signature=";
					authorization += stringToSignSHA256Hex;
					
					S3ParamVector params;
					params.push_back(std::make_pair("x-amz-date", dateTime));
					params.push_back(std::make_pair("x-amz-content-sha256", contentSHA256));
					params.push_back(std::make_pair("x-amz-meta-sha256", dataSHA256Hex));
					params.push_back(std::make_pair("Authorization", authorization));
					
					std::string url("https://");
					url += host;
					url += s3Path;
					url += "?uploads";
					
					auto commandCompletion = std::make_shared<SendCommandCompletion>(url,
																					 redirectCount,
																					 host,
																					 s3Path,
																					 dataSHA256Hex,
																					 awsPublicKey,
																					 awsSigningKey,
																					 awsRegion,
																					 completion);
					SendS3Command(h_,
								  url,
								  method,
								  params,
								  commandCompletion);
				}
			};

		} // namespace InitiateS3MultipartUpload_Impl
		using namespace InitiateS3MultipartUpload_Impl;
		
		//
		void InitiateS3MultipartUpload(const HermitPtr& h_,
									   const std::string& awsPublicKey,
									   const std::string& awsSigningKey,
									   const std::string& awsRegion,
									   const std::string& s3BucketName,
									   const std::string& s3ObjectKey,
									   const std::string& dataSHA256Hex,
									   const InitiateS3MultipartUploadCompletionPtr& completion) {
			std::string region(awsRegion);
			std::string host(s3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string s3Path(s3ObjectKey);
			if ((s3Path.size() > 0) && (s3Path[0] != '/')) {
				s3Path.insert(0, "/");
			}
			
			Redirector::InitiateS3MultipartUpload(h_,
												  0,
												  host,
												  s3Path,
												  dataSHA256Hex,
												  awsPublicKey,
												  awsSigningKey,
												  awsRegion,
												  completion);
		}
		
	} // namespace s3
} // namespace hermit
