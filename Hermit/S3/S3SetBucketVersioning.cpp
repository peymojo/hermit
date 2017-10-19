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
#include "Hermit/String/HexStringToBinary.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SendS3CommandWithData.h"
#include "SignAWSRequestVersion2.h"
#include "S3SetBucketVersioning.h"

namespace hermit {
	namespace s3 {
		
		namespace {

			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//	//
			//	//
			//	std::string BytesToString(
			//		const std::string& inBytes)
			//	{
			//		std::string bytesWithoutSpaces;
			//		for (std::string::size_type n = 0; n < inBytes.size(); ++n)
			//		{
			//			if (inBytes[n] != ' ')
			//			{
			//				bytesWithoutSpaces += inBytes[n];
			//			}
			//		}
			//		BinaryCallbackClassT<std::string> binaryCallback;
			//		HexStringToBinary(inHub, bytesWithoutSpaces, binaryCallback);
			//		return binaryCallback.mValue;
			//	}
			
			//
			//
			class ProcessS3SetBucketVersioningXMLClass : xml::ParseXMLClient 
			{
			private:
				//
				//
				enum ParseState
				{
					kParseState_New,
					kParseState_Error,
					kParseState_Error_Code,
					kParseState_Error_Message,
					kParseState_Error_StringToSignBytes,
					kParseState_ListAllMyBucketsResult,
					kParseState_Buckets,
					kParseState_Bucket,
					kParseState_Name,
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessS3SetBucketVersioningXMLClass()
				:
				mParseState(kParseState_New),
				mIsError(false)
				{
				}
				
				//
				//
				bool GetIsError() const
				{
					return mIsError;
				}
				
				//
				xml::ParseXMLStatus Process(const HermitPtr& h_, const std::string& inXMLData) {
					return xml::ParseXMLData(h_, inXMLData, *this);
				}
				
			private:
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "ListAllMyBucketsResult")
						{
							PushState(kParseState_ListAllMyBucketsResult);
						}
						else if (inStartTag == "Error")
						{
							mIsError = true;
							PushState(kParseState_Error);
						}
						else if (inStartTag != "?xml")
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Error)
					{
						if (inStartTag == "Code")
						{
							PushState(kParseState_Error_Code);
						}
						else if (inStartTag == "Message")
						{
							PushState(kParseState_Error_Message);
						}
						else if (inStartTag == "StringToSignBytes")
						{
							PushState(kParseState_Error_StringToSignBytes);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_ListAllMyBucketsResult)
					{
						if (inStartTag == "Buckets")
						{
							PushState(kParseState_Buckets);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Buckets)
					{
						if (inStartTag == "Bucket")
						{
							PushState(kParseState_Bucket);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else if (mParseState == kParseState_Bucket)
					{
						if (inStartTag == "Name")
						{
							PushState(kParseState_Name);
						}
						else
						{
							PushState(kParseState_IgnoredElement);
						}
					}
					else
					{
						PushState(kParseState_IgnoredElement);
					}
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnContent(const std::string& inContent) override {
					if (mParseState == kParseState_Name)
					{
						//std::cout << inContent << "\n";
					}
					else if (mParseState == kParseState_Error_Code)
					{
						//std::cout << "Error Code: " << inContent << "\n";
					}
					else if (mParseState == kParseState_Error_Message)
					{
						//std::cout << "Error Message: " << inContent << "\n";
					}
					else if (mParseState == kParseState_Error_StringToSignBytes)
					{
						//std::cout << "Error String To Sign Bytes: " << inContent << "\n";
						//std::cout << "Error String To Sign: " << BytesToString(inHub, inContent) << "\n";
					}
					
					return xml::kParseXMLStatus_OK;
				}
				
				//
				virtual xml::ParseXMLStatus OnEnd(const std::string& inEndTag) override {
					PopState();
					return xml::kParseXMLStatus_OK;
				}
				
				//
				//
				void PushState(
							   ParseState inNewState)
				{
					mParseStateStack.push(mParseState);
					mParseState = inNewState;
				}
				
				//
				//
				void PopState()
				{
					mParseState = mParseStateStack.top();
					mParseStateStack.pop();
				}
				
				//
				//
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				bool mIsError;
			};
			
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
						mResponseData = std::string(inData.first, inData.second);
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
		bool S3SetBucketVersioning(const HermitPtr& h_,
								   const std::string& inS3PublicKey,
								   const std::string& inS3PrivateKey,
								   const std::string& inBucketName,
								   const bool& inVersioningEnabled) {
			
			std::string method("PUT");
			
			std::string urlToSign("/");
			urlToSign += inBucketName;
			urlToSign += "/?versioning";
			
			SignAWSRequestVersion2CallbackClass authCallback;
			SignAWSRequestVersion2(inS3PublicKey,
								   inS3PrivateKey,
								   method,
								   "",
								   "",
								   "",
								   urlToSign,
								   authCallback);
			
			if (!authCallback.mSuccess) {
				NOTIFY_ERROR(h_, "S3SetBucketVersioning: SignAWSRequestVersion2 failed for URL:", urlToSign);
				return false;
			}
			
			std::string url("https://");
			url += inBucketName;
			url += ".s3.amazonaws.com/?versioning";
			
			std::string body;
			body += "<VersioningConfiguration xmlns=\"http://s3.amazonaws.com/doc/2006-03-01/\">\n";
			body += "<Status>";
			if (inVersioningEnabled) {
				body += "Enabled";
			}
			else {
				body += "Suspended";
			}
			body += "</Status>\n";
			//	  <MfaDelete>MfaDeleteState</MFADelete>
			body += "</VersioningConfiguration>\n";
			
			StringPairVector params;
			params.push_back(StringPair("Date", authCallback.mDateString));
			params.push_back(StringPair("Authorization", authCallback.mAuthorizationString));
			
			EnumerateStringValuesFunctionClass paramFunction(params);
			SendCommandCallback sendCommandCallback;
			SendS3CommandWithData(h_,
								  url,
								  method,
								  paramFunction,
								  DataBuffer(body.data(), body.size()),
								  sendCommandCallback);
			
			if (sendCommandCallback.mStatus != S3Result::kSuccess) {
				NOTIFY_ERROR(h_, "S3SetBucketVersioning: SendS3CommandWithData failed.");
				return false;
			}
			
			ProcessS3SetBucketVersioningXMLClass pc;
			pc.Process(h_, sendCommandCallback.mResponseData);
			if (pc.GetIsError()) {
				NOTIFY_ERROR(h_, "S3SetBucketVersioning: Response indicated an error.");
				return false;
			}
			return true;
		}
		
	} // namespace s3
} // namespace hermit
