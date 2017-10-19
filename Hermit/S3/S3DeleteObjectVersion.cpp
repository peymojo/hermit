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
#include "Hermit/String/AddTrailingSlash.h"
#include "SendS3Command.h"
#include "SignAWSRequestVersion2.h"
#include "S3DeleteObjectVersion.h"

namespace hermit {
	namespace s3 {
		
		namespace
		{
			//
			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
#if 000
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
				mStatus(kS3RequestResult_Unknown)
				{
				}
				
				//
				//
				bool Function(
							  const S3RequestResult& inStatus,
							  const EnumerateStringValuesFunctionRef& inParamFunction,
							  const std::string& inData,
							  const uint64_t& inDataSize)
				{
					mStatus = inStatus;
					if (inStatus == kS3RequestResult_Success)
					{
						mResponseData = std::string(inData, inDataSize);
					}
					return true;
				}
				
				//
				//
				S3RequestResult mStatus;
				std::string mResponseData;
			};
			
			//
			//
			static char ToHex(
							  uint8_t inValue)
			{
				if (inValue < 10)
				{
					return '0' + inValue;
				}
				return 'A' + (inValue - 10);
			}
			
			//
			//
			static std::string URLAllButSlashes(
												const char* inStringToEncode)
			{
				std::string result;
				std::string stringToEncode(inStringToEncode);
				for (std::string::size_type i = 0; i < stringToEncode.size(); ++i)
				{
					uint8_t ch = stringToEncode[i];
					if (ch == ' ')
					{
						result += "+";
					}
					else if ((ch > 0x7f) || (ch < ' ') || (ch == '&') || (ch == '+') || (ch == ':') || (ch == '=') || (ch == '{') || (ch == '}') ||
							 (ch == '#') || (ch == '<') || (ch == '>') || (ch == '\'') || (ch == '|') || (ch == '%'))
					{
						result += "%";
						uint8_t char1 = (ch >> 4);
						result.push_back(ToHex(char1));
						uint8_t char2 = (ch & 0xf);
						result.push_back(ToHex(char2));
					}
					else
					{
						result += ch;
					}
				}
				return result;
			}
#endif
			
		} // private namespace
		
		//
		bool S3DeleteObjectVersion(const HermitPtr& h_,
								   const std::string& inAWSPublicKey,
								   const std::string& inAWSSigningKey,
								   const uint64_t& inAWSSigningKeySize,
								   const std::string& inAWSRegion,
								   const std::string& inBucketName,
								   const std::string& inObjectKey,
								   const std::string& inObjectVersion)
		{
			//	needs to be updated to AWS4
			return false;
			
#if 000
			std::string method("DELETE");
			std::string s3Path(inObjectKey);
			if (!s3Path.empty() && (s3Path[0] != '/'))
			{
				s3Path = "/" + s3Path;
			}
			
			s3Path = URLAllButSlashes(s3Path);
			
			std::string contentType;
			std::string urlToSign("/");
			urlToSign += inBucketName;
			urlToSign += s3Path;
			urlToSign += "?versionId=";
			urlToSign += inObjectVersion;
			
			SignAWSRequestVersion2CallbackClassT<std::string> authCallback;
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
				LogString(
						  "S3DeleteObjectVersion: SignAWSRequestVersion2 failed for URL: ",
						  urlToSign);
				
				inCallback.Call(false);
				return;
			}
			
			StringPairVector params;
			params.push_back(StringPair("Date", authCallback.mDateString));
			params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
			
			std::string url("https://");
			url += inBucketName;
			url += ".s3.amazonaws.com";
			url += s3Path;
			url += "?versionId=";
			url += inObjectVersion;
			
			EnumerateStringValuesFunctionClassT<StringPairVector> headerParams(params);
			SendCommandCallback result;
			SendS3Command(url, method, headerParams, h_, result);
			if (result.mStatus != kS3RequestResult_Success)
			{
				LogString(
						  "S3DeleteObjectVersion: SendS3Command failed for URL: ",
						  url);
				
				inCallback.Call(false);
				return;
			}
			inCallback.Call(true);
#endif
		}
		
	} // namespace s3
} // namespace hermit
