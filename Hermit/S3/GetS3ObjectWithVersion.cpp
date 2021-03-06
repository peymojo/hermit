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
#include "Hermit/Encoding/CalculateMD5.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/HTTP/CreateHTTPSession.h"
#include "SignAWSRequestVersion2.h"
#include "GetS3ObjectWithVersion.h"

namespace hermit {
	namespace s3 {
		namespace GetS3ObjectWithVersion_Impl {
			
			//
			class CompletionBlock : public http::HTTPRequestCompletionBlock {
			public:
				//
				CompletionBlock(const http::HTTPSessionPtr& httpSession,
								const std::string& url,
								const http::HTTPRequestResponsePtr& httpResponse,
								const GetS3ObjectResponseBlockPtr& s3Response,
								const S3CompletionBlockPtr& s3Completion) :
				mHTTPSession(httpSession),
				mURL(url),
				mHTTPResponse(httpResponse),
				mS3Response(s3Response),
				mS3Completion(s3Completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const http::HTTPRequestResult& result) override {
					if (result != http::HTTPRequestResult::kSuccess) {
						if (result == http::HTTPRequestResult::kCanceled) {
							mS3Completion->Call(h_, S3Result::kCanceled);
							return;
						}
						if (result == http::HTTPRequestResult::kTimedOut) {
							NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: SendHTTPRequest timed out for URL:", mURL);
						}
						else {
							NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: SendHTTPRequest failed for URL:", mURL);
						}
						mS3Completion->Call(h_, S3Result::kError);
						return;
					}
					if (mHTTPResponse->mStatusCode != 200) {
						NOTIFY_ERROR(h_,
									 "GetS3ObjectWithVersion: responseCode != 200 for URL:", mURL,
									 "responseCode:", mHTTPResponse->mStatusCode,
									 "response:", mHTTPResponse->mResponseData);
						mS3Completion->Call(h_, S3Result::kError);
						return;
					}
					
					//	??? this code needs to be updated to use x-amz-meta sha256 if available?
					NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: Etag as proxy for checksum is unreliable, URL:", mURL);
					mS3Completion->Call(h_, S3Result::kError);
					return;
					
					
#if 000
					std::string s3md5Hex;
					auto end = result.mParams.end();
					for (auto it = result.mParams.begin(); it != end; ++it)
					{
						if (it->first == "Etag")
						{
							std::string tag(it->second);
							if (!tag.empty())
							{
								if (tag[0] == '"')
								{
									tag = tag.substr(1);
								}
								if (!tag.empty())
								{
									if (tag[tag.size() - 1] == '"')
									{
										tag = tag.substr(0, tag.size() - 1);
									}
								}
								s3md5Hex = tag;
							}
						}
					}
					if (s3md5Hex.empty())
					{
						NOTIFY_ERROR("GetS3ObjectWithVersion: Etag not found, URL:", url);
						inCallback.Call(S3Result::kError, nullptr, 0);
						return;
					}
					
					encoding::CalculateMD5CallbackClassT<std::string> md5Callback;
					encoding::CalculateMD5(result.mResponse.data(), result.mResponse.size(), md5Callback);
					if (md5Callback.mStatus == encoding::kCalculateMD5Status_Canceled)
					{
						inCallback.Call(S3Result::kCanceled, nullptr, 0);
						return;
					}
					if (md5Callback.mStatus != encoding::kCalculateMD5Status_Success)
					{
						NOTIFY_ERROR("GetS3ObjectWithVersion: CalculateMD5 failed, URL:", url);
						inCallback.Call(S3Result::kError, nullptr, 0);
						return;
					}
					
					if (md5Callback.mMD5Hex != s3md5Hex)
					{
						inCallback.Call(S3Result::kChecksumMismatch, nullptr, 0);
						return;
					}
					
					inCallback.Call(S3Result::kSuccess, result.mResponse.data(), result.mResponse.size());
#endif
				}
				
				//
				http::HTTPSessionPtr mHTTPSession;
				std::string mURL;
				http::HTTPRequestResponsePtr mHTTPResponse;
				GetS3ObjectResponseBlockPtr mS3Response;
				S3CompletionBlockPtr mS3Completion;
			};
			
		} // namespace GetS3ObjectWithVersion_Impl
		using namespace GetS3ObjectWithVersion_Impl;
		
		//
		void GetS3ObjectWithVersion(const HermitPtr& h_,
									const std::string& inS3BucketName,
									const std::string& inS3ObjectKey,
									const std::string& inS3PublicKey,
									const std::string& inS3PrivateKey,
									const std::string& inVersion,
									const GetS3ObjectResponseBlockPtr& inResponseBlock,
									const S3CompletionBlockPtr& inCompletion) {
			std::string method("GET");
			std::string s3Path(inS3ObjectKey);
			if (!s3Path.empty() && (s3Path[0] != '/')) {
				s3Path = "/" + s3Path;
			}
			
			std::string contentType;
			
			std::string urlToSign("/");
			urlToSign += inS3BucketName;
			urlToSign += s3Path;
			urlToSign += "?versionId=";
			urlToSign += inVersion;
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(inS3PublicKey,
								   inS3PrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess) {
				NOTIFY_ERROR(h_, "GetS3ObjectWithVersion: SignAWSRequestVersion2 failed for URL:", urlToSign);
				inCompletion->Call(h_, S3Result::kError);
				return;
			}
			
			http::HTTPParamVector params;
			params.push_back(std::make_pair("Date", authCallback.mDateString));
			params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += inS3BucketName;
			url += ".s3.amazonaws.com";
			url += s3Path;
			url += "?versionId=";
			url += inVersion;
			
			http::HTTPSessionPtr httpSession(http::CreateHTTPSession());
			auto response = std::make_shared<http::HTTPRequestResponse>();
			auto completion = std::make_shared<CompletionBlock>(httpSession, url, response, inResponseBlock, inCompletion);
			httpSession->SendRequest(h_, url, method, params, response, completion);
		}
		
	} // namespace s3
} // namespace hermit
