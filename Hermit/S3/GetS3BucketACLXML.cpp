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
		
		namespace
		{
			//
			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			//
			class SendCommandCallback
			:
			public SendS3CommandCallback
			{
			public:
				//
				//
				SendCommandCallback()
				:
				mStatus(S3Result::kUnknown)
				{
				}
				
				//
				//
				bool Function(
							  const S3Result& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const DataBuffer& inData)
				{
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess)
					{
						mResponse = std::string(inData.first, inData.second);
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponse;
			};
			
		} // private namespace
		
		//
		//
		void GetS3BucketACLXML(const HermitPtr& h_,
							   const std::string& inBucketName,
							   const std::string& inS3PublicKey,
							   const std::string& inS3PrivateKey,
							   const GetS3BucketACLXMLCallbackRef& inCallback)
		{
			std::string method("GET");
			std::string contentType;
			std::string urlToSign("/");
			urlToSign += inBucketName;
			urlToSign += "/?acl";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(
								   inS3PublicKey,
								   inS3PrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess)
			{
				NOTIFY_ERROR(h_, "GetS3BucketACLXML: SignAWSRequestVersion2 failed for URL:", urlToSign);
				inCallback.Call(kGetS3BucketACLXMLResult_Error, "");
				return;
			}
			
			StringPairVector params;
			params.push_back(StringPair("Date", authCallback.mDateString));
			params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += inBucketName;
			url += ".s3.amazonaws.com/?acl";
			
			EnumerateStringValuesFunctionClass headerParams(params);
			SendCommandCallback result;
			SendS3Command(h_, url, method, headerParams, result);
			if (result.mStatus != S3Result::kSuccess)
			{
				if (result.mStatus == S3Result::k404EntityNotFound)
				{
					inCallback.Call(kGetS3BucketACLXMLResult_NoSuchBucket, "");
				}
				else
				{
					NOTIFY_ERROR(h_, "GetS3BucketACLXML: SendS3Command failed for URL:", url);
					inCallback.Call(kGetS3BucketACLXMLResult_Error, "");
				}
			}
			else
			{
				inCallback.Call(kGetS3BucketACLXMLResult_Success, result.mResponse);
			}
		}
		
	} // namespace s3
} // namespace hermit
