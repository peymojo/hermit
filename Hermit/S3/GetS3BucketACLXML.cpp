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
#include "Hermit/Foundation/Notification.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "GetS3BucketACLXML.h"

namespace hermit {
	namespace s3 {
		namespace GetS3BucketACLXML_Impl {
			
			//
			class CommandCompletion : public SendS3CommandCompletion {
			public:
				//
				CommandCompletion(const std::string& url,
								  const GetS3BucketACLXMLCompletionPtr& completion) :
				mURL(url),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const S3Result& result,
								  const S3ParamVector& params,
								  const DataBuffer& data) override {
					if (result != S3Result::kSuccess) {
						if (result == S3Result::k404EntityNotFound) {
							mCompletion->Call(h_, GetS3BucketACLXMLResult::kNoSuchBucket, "");
							return;
						}

						NOTIFY_ERROR(h_, "GetS3BucketACLXML: SendS3Command failed for URL:", mURL);
						mCompletion->Call(h_, GetS3BucketACLXMLResult::kError, "");
						return;
					}
					mCompletion->Call(h_, GetS3BucketACLXMLResult::kSuccess, std::string(data.first, data.second));
				}
				
				//
				std::string mURL;
				GetS3BucketACLXMLCompletionPtr mCompletion;
			};
			
		} // namespace GetS3BucketACLXML_Impl
		using namespace GetS3BucketACLXML_Impl;
		
		//
		void GetS3BucketACLXML(const HermitPtr& h_,
							   const std::string& bucketName,
							   const std::string& s3PublicKey,
							   const std::string& s3PrivateKey,
							   const GetS3BucketACLXMLCompletionPtr& completion) {
			std::string method("GET");
			std::string contentType;
			std::string urlToSign("/");
			urlToSign += bucketName;
			urlToSign += "/?acl";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(s3PublicKey,
								   s3PrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess) {
				NOTIFY_ERROR(h_, "GetS3BucketACLXML: SignAWSRequestVersion2 failed for URL:", urlToSign);
				completion->Call(h_, GetS3BucketACLXMLResult::kError, "");
				return;
			}
			
			S3ParamVector params;
			params.push_back(std::make_pair("Date", authCallback.mDateString));
			params.push_back(std::make_pair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += bucketName;
			url += ".s3.amazonaws.com/?acl";
			
			auto commandCompletion = std::make_shared<CommandCompletion>(url, completion);
			SendS3Command(h_, url, method, params, commandCompletion);
		}
		
	} // namespace s3
} // namespace hermit
