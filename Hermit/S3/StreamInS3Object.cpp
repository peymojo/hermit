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
#include "Hermit/HTTP/URLEncode.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SignAWSRequestVersion2.h"
#include "StreamInS3Request.h"
#include "StreamInS3Object.h"

namespace hermit {
	namespace s3 {
		namespace StreamInS3Object_Impl {
			
			//
			std::string GetSHA256(const S3ParamVector& params) {
				auto end = params.end();
				for (auto it = params.begin(); it != end; ++it) {
					if ((*it).first == "x-amz-meta-sha256") {
						return (*it).second;
					}
				}
				return "";
			}

			//
			class Receiver : public DataReceiver {
			public:
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<Receiver> ReceiverPtr;

			//
			class ProcessXMLClass : xml::ParseXMLClient {
			private:
				//
				enum class ParseState {
					kNew,
					kError,
					kCode,
					kEndpoint,
					kIgnoredElement
				};
				
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				ProcessXMLClass(const HermitPtr& h_) :
				mH_(h_),
				mParseState(ParseState::kNew) {
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
						if (inStartTag == "Error") {
							PushState(ParseState::kError);
						}
						else if (inStartTag != "?xml") {
							PushState(ParseState::kIgnoredElement);
						}
					}
					else if (mParseState == ParseState::kError) {
						if (inStartTag == "Code") {
							PushState(ParseState::kCode);
						}
						else if (inStartTag == "Endpoint") {
							PushState(ParseState::kEndpoint);
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
					if (mParseState == ParseState::kCode) {
						mCode = inContent;
					}
					else if (mParseState == ParseState::kEndpoint) {
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
				std::string mCode;
				std::string mEndpoint;
			};
			
			//
			class Redirector {
				//
				class StreamInCompletion : public StreamInS3RequestCompletion {
				public:
					//
					StreamInCompletion(const http::HTTPSessionPtr& session,
									   const std::string& url,
									   int redirectCount,
									   const std::string& host,
									   const std::string& s3Path,
									   const std::string& awsPublicKey,
									   const std::string& awsSigningKey,
									   const std::string& awsRegion,
									   const ReceiverPtr& ourDataReceiver,
									   const DataReceiverPtr& theirDataReceiver,
									   const S3CompletionBlockPtr& completion) :
					mSession(session),
					mURL(url),
					mRedirectCount(redirectCount),
					mHost(host),
					mS3Path(s3Path),
					mAWSPublicKey(awsPublicKey),
					mAWSSigningKey(awsSigningKey),
					mAWSRegion(awsRegion),
					mOurDataReceiver(ourDataReceiver),
					mTheirDataReceiver(theirDataReceiver),
					mCompletion(completion) {
					}
					
					//
					class Completion : public DataCompletion {
					public:
						//
						Completion(const S3CompletionBlockPtr& completion) : mCompletion(completion) {
						}
						
						//
						virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
							if (result == StreamDataResult::kCanceled) {
								mCompletion->Call(h_, S3Result::kCanceled);
								return;
							}
							if (result != StreamDataResult::kSuccess) {
								NOTIFY_ERROR(h_, "StreamData failed.");
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
							mCompletion->Call(h_, S3Result::kSuccess);
						}
						
						//
						S3CompletionBlockPtr mCompletion;
					};
					
					//
					virtual void Call(const HermitPtr& h_, const S3Result& result, const S3ParamVector& params) override {
						if (result != S3Result::kSuccess) {
							if (result == S3Result::kCanceled) {
								mCompletion->Call(h_, S3Result::kCanceled);
								return;
							}
							if (result == S3Result::kAuthorizationHeaderMalformed) {
								mCompletion->Call(h_, S3Result::kAuthorizationHeaderMalformed);
								return;
							}
							if (result == S3Result::k400BadRequest) {
								mCompletion->Call(h_, S3Result::k400BadRequest);
								return;
							}
							if (result == S3Result::k403AccessDenied) {
								mCompletion->Call(h_, S3Result::k403AccessDenied);
								return;
							}
							if (result == S3Result::k404EntityNotFound) {
								mCompletion->Call(h_, S3Result::k404EntityNotFound);
								return;
							}
							if (result == S3Result::k404NoSuchBucket) {
								mCompletion->Call(h_, S3Result::k404NoSuchBucket);
								return;
							}
							if (result == S3Result::kNetworkConnectionLost) {
								mCompletion->Call(h_, S3Result::kNetworkConnectionLost);
								return;
							}
							if ((result == S3Result::kS3InternalError) ||
								(result == S3Result::k500InternalServerError) ||
								(result == S3Result::k503ServiceUnavailable)) {
								mCompletion->Call(h_, result);
								return;
							}
							if (result == S3Result::kTimedOut) {
								mCompletion->Call(h_, S3Result::kTimedOut);
								return;
							}
							if (result == S3Result::k301PermanentRedirect) {
								mCompletion->Call(h_, S3Result::k301PermanentRedirect);
								return;
							}
							if (result == S3Result::k307TemporaryRedirect) {
								ProcessXMLClass pc(h_);
								pc.Process(mOurDataReceiver->mData);
								if (pc.mCode == "TemporaryRedirect") {
									if (pc.mEndpoint.empty()) {
										NOTIFY_ERROR(h_,
													 "S3Result::k307TemporaryRedirect but new endpoint is empty for host:",
													 mHost);
										mCompletion->Call(h_, S3Result::kError);
										return;
									}
									if (pc.mEndpoint == mHost) {
										NOTIFY_ERROR(h_,
													 "S3Result::k307TemporaryRedirect but new endpoint is the same for host:",
													 mHost);
										mCompletion->Call(h_, S3Result::kError);
										return;
									}
									// Reset the data buffer, otherwise the result of the redirect will be appended.
									mOurDataReceiver->mData.clear();
									StreamInS3Object(h_,
													 mSession,
													 mRedirectCount + 1,
													 pc.mEndpoint,
													 mS3Path,
													 mAWSPublicKey,
													 mAWSSigningKey,
													 mAWSRegion,
													 mOurDataReceiver,
													 mTheirDataReceiver,
													 mCompletion);
									return;
								}

								NOTIFY_ERROR(h_,
											 "Unparsed 307 Temporary Redirect for host:", mHost,
											 "response:", mOurDataReceiver->mData);
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
							NOTIFY_ERROR(h_, "StreamInS3Request failed for URL:", mURL);
							mCompletion->Call(h_, result);
							return;
						}

						//	We currently put this value when we put an s3 object, but we can't assume it's
						//	there for any given s3 object we're asked to fetch. So this checksum step is optional.
						std::string s3sha256hex(GetSHA256(params));
						if (!s3sha256hex.empty()) {
							std::string dataSHA256;
							encoding::CalculateSHA256(mOurDataReceiver->mData, dataSHA256);
							if (dataSHA256.empty()) {
								NOTIFY_ERROR(h_, "CalculateSHA256 failed.");
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
							std::string dataSHA256Hex;
							string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
							if (dataSHA256Hex.empty()) {
								NOTIFY_ERROR(h_, "BinaryStringToHex failed.");
								mCompletion->Call(h_, S3Result::kError);
								return;
							}
								
							if (s3sha256hex != dataSHA256Hex) {
								NOTIFY_ERROR(h_,
											 "checksum mismatch for for URL:", mURL,
											 "s3 value:", s3sha256hex,
											 "local value:", dataSHA256Hex);
									
								mCompletion->Call(h_, S3Result::kChecksumMismatch);
								return;
							}
						}
							
						auto buffer = DataBuffer(mOurDataReceiver->mData.data(), mOurDataReceiver->mData.size());
						auto receiveCompletion = std::make_shared<Completion>(mCompletion);
						mTheirDataReceiver->Call(h_, buffer, true, receiveCompletion);
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
					ReceiverPtr mOurDataReceiver;
					DataReceiverPtr mTheirDataReceiver;
					S3CompletionBlockPtr mCompletion;
				};
				
			public:
				//
				static void StreamInS3Object(const HermitPtr& h_,
											 const http::HTTPSessionPtr& session,
											 int redirectCount,
											 const std::string& host,
											 const std::string& s3Path,
											 const std::string& awsPublicKey,
											 const std::string& awsSigningKey,
											 const std::string& awsRegion,
											 const ReceiverPtr& ourDataReceiver,
											 const DataReceiverPtr& theirDataReceiver,
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
					
					std::string contentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
					
					std::string method("GET");
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
					authorization += "SignedHeaders=host;x-amz-content-sha256;x-amz-date";
					authorization += ",";
					authorization += "Signature=";
					authorization += stringToSignSHA256Hex;
					
					S3ParamVector params;
					params.push_back(std::make_pair("x-amz-date", dateTime));
					params.push_back(std::make_pair("x-amz-content-sha256", contentSHA256));
					params.push_back(std::make_pair("Authorization", authorization));
					
					std::string url("https://");
					url += host;
					url += s3Path;
					
					auto streamCompletion = std::make_shared<StreamInCompletion>(session,
																				 url,
																				 redirectCount,
																				 host,
																				 s3Path,
																				 awsPublicKey,
																				 awsSigningKey,
																				 awsRegion,
																				 ourDataReceiver,
																				 theirDataReceiver,
																				 completion);
					StreamInS3Request(h_, session, url, method, params, ourDataReceiver, streamCompletion);
				}
			};
			
		} // namespace StreamInS3Object_Impl
		using namespace StreamInS3Object_Impl;
		
		//
		void StreamInS3Object(const HermitPtr& h_,
							  const http::HTTPSessionPtr& session,
							  const std::string& awsPublicKey,
							  const std::string& awsSigningKey,
							  const std::string& awsRegion,
							  const std::string& s3BucketName,
							  const std::string& s3ObjectKey,
							  const DataReceiverPtr& dataReceiver,
							  const S3CompletionBlockPtr& completion) {
			std::string host(s3BucketName);
			host += ".s3.amazonaws.com";
			
			std::string urlEncodedObjectKey;
			http::URLEncode(s3ObjectKey, false, urlEncodedObjectKey);
			std::string s3Path(urlEncodedObjectKey);
			if (!s3Path.empty() && (s3Path[0] != '/')) {
				s3Path = "/" + s3Path;
			}
			
			auto ourDataReceiver = std::make_shared<Receiver>();
			Redirector::StreamInS3Object(h_,
										 session,
										 0,
										 host,
										 s3Path,
										 awsPublicKey,
										 awsSigningKey,
										 awsRegion,
										 ourDataReceiver,
										 dataReceiver,
										 completion);
		}
		
	} // namespace s3
} // namespace hermit
