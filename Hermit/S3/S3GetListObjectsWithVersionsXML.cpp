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
#include "Hermit/HTTP/URLEncode.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "S3GetListObjectsWithVersionsXML.h"

namespace hermit {
	namespace s3 {
		
		//
		//
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
							  const std::string& inData,
							  const uint64_t& inDataSize)
				{
					mStatus = inStatus;
					if (inStatus == S3Result::kSuccess)
					{
						mResponseData = std::string(inData, inDataSize);
					}
					return true;
				}
				
				//
				//
				S3Result mStatus;
				std::string mResponseData;
			};
			
		} // private namespace
		
		//
		//
		void S3GetListObjectsWithVersionsXML(const HermitPtr& h_,
											 const std::string& inAWSPublicKey,
											 const std::string& inAWSSigningKey,
											 const uint64_t& inAWSSigningKeySize,
											 const std::string& inAWSRegion,
											 const std::string& inBucketName,
											 const std::string& inBasePath,
											 const std::string& inMarker,
											 const S3GetListObjectsWithVersionsXMLCallbackRef& inCallback)
		{
			//	needs to be updated to AWS4
			inCallback.Call(h_, false, "");
			
#if 000
			std::string bucketName(inBucketName);
			
			StringCallbackClass stringCallback;
			http::URLEncode(inMarker, true, stringCallback);
			std::string marker(stringCallback.mValue);
			
			std::string method("GET");
			
			std::string contentType;
			
			std::string urlToSign("/");
			urlToSign += bucketName + "/?versions";
			
			SignAWSRequestVersion2CallbackClassT<std::string> authCallback;
			SignAWSRequestVersion2(
								   inAWSPublicKey,
								   inAWSPrivateKey,
								   method,
								   "",
								   contentType,
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess)
			{
				LogString(
						  "S3GetListObjectsWithVersionsXML: SignAWSRequestVersion2 failed for URL: ",
						  urlToSign);
				
				inCallback.Call(false, "");
				return;
			}
			
			StringPairVector params;
			params.push_back(StringPair("Date", authCallback.mDateString));
			params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += bucketName;
			url += ".s3.amazonaws.com/?versions";
			//	url += "s3.amazon.com/";
			//	url += bucketName;
			//	url += "?versions";
			if (!marker.empty())
			{
				url += "&key-marker=";
				url += marker;
			}
			if (strlen(inBasePath) > 0)
			{
				url += "&prefix=";
				url += inBasePath;
			}
			
			EnumerateStringValuesFunctionClassT<StringPairVector> s3Params(params);
			SendCommandCallback result;
			SendS3Command(url, method, s3Params, h_, result);
			if (result.mStatus != S3Result::kSuccess)
			{
				LogString(
						  "S3GetListObjectsWithVersionsXML(): SendS3Command failed for: ",
						  url);
				
				inCallback.Call(false, "");
			}
			else
			{
				inCallback.Call(true, result.mResponseData);
			}
#endif
		}
		
	} // namespace s3
} // namespace hermit
