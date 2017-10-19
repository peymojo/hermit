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

#if 000

#include <stack>
#include <string>
#include <vector>
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/HTTP/SendHTTPRequestWithBody.h"
#include "Hermit/HTTP/URLEncode.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/String/UInt64ToString.h"
#include "Hermit/XML/ParseXMLData.h"
#include "SignAWSRequestVersion2.h"
#include "PutS3Object.h"

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
	class ProcessErrorXMLClass
	{
	private:
		//
		//
		enum ParseState
		{
			kParseState_New,
			kParseState_Error,
			kParseState_Code,
			kParseState_Endpoint,
			kParseState_Region,
			kParseState_IgnoredElement
		};
		
		//
		//
		typedef std::stack<ParseState> ParseStateStack;
		
	public:
		//
		//
		ProcessErrorXMLClass()
			:
			mParseState(kParseState_New)
		{
		}
		
		//
		//
		class OnStartFunction
			:
			public xml::ParseXMLOnStartFunction
		{
		public:
			//
			//
			OnStartFunction(
				ProcessErrorXMLClass& inClass)
				:
				mClass(inClass)
			{
			}
			
			//
			//
			bool Function(
				const std::string& inStartTag,
				const std::string& inAttributes,
				const bool& inIsEmptyElement,
				const xml::ParseXMLCallbackRef& inCallback)
			{
				return mClass.ParseXMLOnStart(inStartTag, inAttributes, inIsEmptyElement, inCallback);
			}
			
			//
			//
			ProcessErrorXMLClass& mClass;
		};

		//
		//
		class OnContentFunction
			:
			public xml::ParseXMLOnContentFunction
		{
		public:
			//
			//
			OnContentFunction(
				ProcessErrorXMLClass& inClass)
				:
				mClass(inClass)
			{
			}
			
			//
			//
			bool Function(
				const std::string& inContent,
				const xml::ParseXMLCallbackRef& inCallback)
			{
				return mClass.ParseXMLOnContent(inContent, inCallback);
			}
			
			//
			//
			ProcessErrorXMLClass& mClass;
		};

		//
		//
		class OnEndFunction
			:
			public xml::ParseXMLOnEndFunction
		{
		public:
			//
			//
			OnEndFunction(
				ProcessErrorXMLClass& inClass)
				:
				mClass(inClass)
			{
			}
			
			//
			//
			bool Function(
				const std::string& inEndTag,
				const xml::ParseXMLCallbackRef& inCallback)
			{
				return mClass.ParseXMLOnEnd(inEndTag, inCallback);
			}
			
			//
			//
			ProcessErrorXMLClass& mClass;
		};

		//
		//
		void Process(
			const std::string& inXMLData)
		{
			OnStartFunction onStartFunction(*this);
			OnContentFunction onContent(*this);
			OnEndFunction onEnd(*this);
			xml::ParseXMLCallbackClass callback;
			xml::ParseXMLData(
				inXMLData,
				onStartFunction,
				onContent,
				onEnd,
				callback);
		}
		
		//
		//
		bool ParseXMLOnStart(
			const char* inStartTag,
			const char* inAttributes,
			bool inIsEmptyElement,
			const xml::ParseXMLCallbackRef& inCallback)
		{
//			std::cout << "ParseXMLOnStart: inStartTag=|" << inStartTag << "|, inAttributes=|" << inAttributes << "|\n";
			
			if (mParseState == kParseState_New)
			{
				if (strcmp(inStartTag, "Error") == 0)
				{
					PushState(kParseState_Error);
				}
			}
			else if (mParseState == kParseState_Error)
			{
				if (strcmp(inStartTag, "Region") == 0)
				{
					if (inIsEmptyElement)
					{
						mLocation = "us-east-1";
					}
					else
					{
						PushState(kParseState_Region);
					}
				}
				else if (strcmp(inStartTag, "Code") == 0)
				{
					PushState(kParseState_Code);
				}
				else if (strcmp(inStartTag, "Endpoint") == 0)
				{
					PushState(kParseState_Endpoint);
				}
				else if (strcmp(inStartTag, "?xml") != 0)
				{
					PushState(kParseState_IgnoredElement);
				}
			}
			else
			{
				PushState(kParseState_IgnoredElement);
			}
			inCallback.Call(xml::kParseXMLStatus_OK);
			return true;
		}
		
		//
		//
		bool ParseXMLOnContent(
			const char* inContent,
			const xml::ParseXMLCallbackRef& inCallback)
		{
			if (mParseState == kParseState_Region)
			{
				mLocation = inContent;
			}
			else if (mParseState == kParseState_Code)
			{
				mCode = inContent;
			}
			else if (mParseState == kParseState_Endpoint)
			{
				mEndpoint = inContent;
			}
			inCallback.Call(xml::kParseXMLStatus_OK);
			return true;
		}
		
		//
		//
		bool ParseXMLOnEnd(
			const char* inEndTag,
			const xml::ParseXMLCallbackRef& inCallback)
		{
//			std::cout << "ParseXMLOnEnd: inEndTag=|" << inEndTag << "|\n";
			PopState();
			inCallback.Call(xml::kParseXMLStatus_OK);
			return true;
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
		std::string mCode;
		std::string mLocation;
		std::string mEndpoint;
	};

} // private namespace

//
//
void PutS3ObjectUsingChunks(
	const std::string& inAWSPublicKey,
	const std::string& inAWSSigningKey,
	const uint64_t& inAWSSigningKeySize,
	const std::string& inAWSRegion,
	const std::string& inS3BucketName,
	const std::string& inS3ObjectKey,
	const DataPtr& inData,
	const bool& inUseReducedRedundancyStorage,
	const ProgressFunctionPtr& inProgressFunction,
	const PutS3ObjectCallbackRef& inCallback)
{
	std::string region(inAWSRegion);
	std::string method("PUT");
	
	std::string host(inS3BucketName);
	host += ".s3.amazonaws.com";
	
	StringCallbackClass urlEncodedS3ObjectKey;
	http::URLEncode(inS3ObjectKey, false, urlEncodedS3ObjectKey);
	std::string s3Path(urlEncodedS3ObjectKey.mValue);
	if (!s3Path.empty() && (s3Path[0] != '/'))
	{
		s3Path = "/" + s3Path;
	}

	int redirectCount = 0;
	while (true)
	{
		time_t now;
		time(&now);
		tm globalTime;
		gmtime_r(&now, &globalTime);
		char dateBuf[2048];
		strftime(dateBuf, 2048, "%Y%m%dT%H%M%SZ", &globalTime);
		std::string dateTime(dateBuf);
		std::string date(dateTime.substr(0, 8));
	
		std::string canonicalRequest(method);
		canonicalRequest += "\n";
		canonicalRequest += s3Path;
		canonicalRequest += "\n";
		canonicalRequest += "\n";
	
		canonicalRequest += "content-encoding:aws-chunked\n";



		char dataSizeHex[256];
		sprintf(dataSizeHex, "%llX", inDataSize);



		size_t metadataSize = strlen(dataSizeHex);
		metadataSize += strlen(";");
		metadataSize += strlen("chunk-signature=");
		metadataSize += 64;
		metadataSize += 2;
	
		metadataSize += 2;

		metadataSize += 1;
		metadataSize += strlen(";");
		metadataSize += strlen("chunk-signature=");
		metadataSize += 64;
		metadataSize += 2;
	
		metadataSize += 2;


		StringCallbackClass totalContentSizeStr;
		string::UInt64ToString(inDataSize + metadataSize, totalContentSizeStr);
		canonicalRequest += "content-size:";
		canonicalRequest += totalContentSizeStr.mValue;
		canonicalRequest += "\n";
	
		canonicalRequest += "host:";
		canonicalRequest += host;
		canonicalRequest += "\n";
	
		canonicalRequest += "x-amz-content-sha256:";
		canonicalRequest += "STREAMING-AWS4-HMAC-SHA256-PAYLOAD";
		canonicalRequest += "\n";

		canonicalRequest += "x-amz-date:";
		canonicalRequest += dateTime;
		canonicalRequest += "\n";

		StringCallbackClass sizeStr;
		string::UInt64ToString(inDataSize, sizeStr);
		canonicalRequest += "x-amz-decoded-content-length:";
		canonicalRequest += sizeStr.mValue;
		canonicalRequest += "\n";

		canonicalRequest += "\n";
		canonicalRequest += "content-encoding;content-size;host;x-amz-content-sha256;x-amz-date;x-amz-decoded-content-length";
		canonicalRequest += "\n";

		canonicalRequest += "STREAMING-AWS4-HMAC-SHA256-PAYLOAD";

		if (inUseReducedRedundancyStorage)
		{
			//	this is wrong but should trigger an error which I can then fix
			canonicalRequest = "x-amz-storage-class:REDUCED_REDUNDANCY";
		}
	
	
	
	
		BinaryCallbackClassT<std::string> canonicalRequestSHA256;
		encoding::CalculateSHA256(canonicalRequest.data(), canonicalRequest.size(), canonicalRequestSHA256);
		StringCallbackClass canonicalRequestSHA256Hex;
		string::BinaryStringToHex(canonicalRequestSHA256.mValue, canonicalRequestSHA256.mValue.size(), canonicalRequestSHA256Hex);
	
	
	
	
		std::string stringToSign("AWS4-HMAC-SHA256");
		stringToSign += "\n";
		stringToSign += dateTime;
		stringToSign += "\n";
		stringToSign += date;
		stringToSign += "/";
		stringToSign += region;
		stringToSign += "/s3/aws4_request";
		stringToSign += "\n";
		stringToSign += canonicalRequestSHA256Hex.mValue;
	
	
	
	
		BinaryCallbackClassT<std::string> signature;
		encoding::CalculateHMACSHA256(
			inAWSSigningKey,
			inAWSSigningKeySize,
			stringToSign.data(),
			stringToSign.size(),
			signature);
		
		StringCallbackClass signatureHex;
		string::BinaryStringToHex(signature.mValue, signatureHex);

		std::string authorization("AWS4-HMAC-SHA256 Credential=");
		authorization += inAWSPublicKey;
		authorization += "/";
		authorization += date;
		authorization += "/";
		authorization += region;
		authorization += "/s3/aws4_request";
		authorization += ",";
		authorization += "SignedHeaders=content-encoding;content-size;host;x-amz-content-sha256;x-amz-date;x-amz-decoded-content-length";
		authorization += ",";
		authorization += "Signature=";
		authorization += signatureHex.mValue;



		std::string chunk1StringToSign("AWS4-HMAC-SHA256-PAYLOAD");
		chunk1StringToSign += "\n";
		chunk1StringToSign += dateTime;
		chunk1StringToSign += "\n";
		chunk1StringToSign += date;
		chunk1StringToSign += "/";
		chunk1StringToSign += region;
		chunk1StringToSign += "/s3/aws4_request";
		chunk1StringToSign += "\n";
		chunk1StringToSign += signatureHex.mValue;
		chunk1StringToSign += "\n";

		std::string nullContentSHA256("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
		chunk1StringToSign += nullContentSHA256;
		chunk1StringToSign += "\n";

		BinaryCallbackClassT<std::string> binaryCallback;
		encoding::CalculateSHA256(inData, inDataSize, binaryCallback);

		StringCallbackClass dataSHA256HexString;
		string::BinaryStringToHex(binaryCallback.mValue, dataSHA256HexString);

		chunk1StringToSign += dataSHA256HexString.mValue;




		BinaryCallbackClassT<std::string> chunk1Signature;
		encoding::CalculateHMACSHA256(
			inAWSSigningKey,
			inAWSSigningKeySize,
			chunk1StringToSign.data(),
			chunk1StringToSign.size(),
			chunk1Signature);
		
		StringCallbackClass chunk1SignatureHex;
		string::BinaryStringToHex(chunk1Signature.mValue, chunk1SignatureHex);


	
		std::string block1Header(dataSizeHex);
		block1Header += ";";
		block1Header += "chunk-signature=";
		block1Header += chunk1SignatureHex.mValue;
		block1Header += "\r\n";





		std::string chunk2StringToSign("AWS4-HMAC-SHA256-PAYLOAD");
		chunk2StringToSign += "\n";
		chunk2StringToSign += dateTime;
		chunk2StringToSign += "\n";
		chunk2StringToSign += date;
		chunk2StringToSign += "/";
		chunk2StringToSign += region;
		chunk2StringToSign += "/s3/aws4_request";
		chunk2StringToSign += "\n";
		chunk2StringToSign += chunk1SignatureHex.mValue;
		chunk2StringToSign += "\n";

		chunk2StringToSign += nullContentSHA256;
		chunk2StringToSign += "\n";

		chunk2StringToSign += nullContentSHA256;




		BinaryCallbackClassT<std::string> chunk2Signature;
		encoding::CalculateHMACSHA256(
			inAWSSigningKey,
			inAWSSigningKeySize,
			chunk2StringToSign.data(),
			chunk2StringToSign.size(),
			chunk2Signature);
		
		StringCallbackClass chunk2SignatureHex;
		string::BinaryStringToHex(chunk2Signature.mValue, chunk2SignatureHex);


		std::string block2Header("0;");
		block2Header += "chunk-signature=";
		block2Header += chunk2SignatureHex.mValue;
		block2Header += "\r\n";


		std::string payload;
		payload.reserve(block1Header.size() + inDataSize + 2 + block2Header.size() + 2);

		payload += block1Header;
		payload.append(inData, inDataSize);
		payload += "\r\n";
		payload += block2Header;
		payload += "\r\n";


		StringPairVector params;
		params.push_back(StringPair("content-encoding", "aws-chunked"));
		params.push_back(StringPair("content-size", totalContentSizeStr.mValue));
		params.push_back(StringPair("x-amz-date", dateTime));

		params.push_back(StringPair("x-amz-content-sha256", "STREAMING-AWS4-HMAC-SHA256-PAYLOAD"));
		params.push_back(StringPair("x-amz-decoded-content-length", sizeStr.mValue));

		params.push_back(StringPair("Authorization", authorization));
//		if (inUseReducedRedundancyStorage)
//		{
//			params.push_back(HTTPPutParam("x-amz-storage-class", "REDUCED_REDUNDANCY"));
//		}

		std::string urlToSign("/");
		urlToSign += host;
		urlToSign += s3Path;

		std::string url("http://");
		url += host;
		url += s3Path;

		EnumerateStringValuesFunctionClassT<StringPairVector> headerParams(params);
		http::SendHTTPRequestCallbackClassT<std::string, StringPairVector> callback;
		http::SendHTTPRequestWithBody(
			url,
			method,
			headerParams,
			payload.data(),
			payload.size(),
			inProgressFunction,
			callback);

		if (callback.mResult != http::kHTTPRequestResult_Success)
		{
			if (callback.mResult == http::kHTTPRequestResult_Canceled)
			{
				inCallback.Call(kPutS3ObjectStatus_Canceled, "");
			}
			else if (callback.mResult == http::kHTTPRequestResult_HostNotFound)
			{
				Log("PutS3Object: kHTTPRequestResult_HostNotFound.");
				inCallback.Call(kPutS3ObjectStatus_HostNotFound, "");
			}
			else if (callback.mResult == http::kHTTPRequestResult_NetworkConnectionLost)
			{
				Log("PutS3Object: kHTTPRequestResult_NetworkConnectionLost.");
				inCallback.Call(kPutS3ObjectStatus_NetworkConnectionLost, "");
			}
			else if (callback.mResult == http::kHTTPRequestResult_TimedOut)
			{
				Log("PutS3Object: kHTTPRequestResult_TimedOut.");
				inCallback.Call(kPutS3ObjectStatus_TimedOut, "");
			}
			else if (callback.mResult == http::kHTTPRequestResult_NoNetworkConnection)
			{
				Log("PutS3Object: kHTTPRequestResult_NoNetworkConnection.");
				inCallback.Call(kPutS3ObjectStatus_NoNetworkConnection, "");
			}
			else
			{
				Log("PutS3Object: SendHTTPRequestWithBody failed.");
				inCallback.Call(kPutS3ObjectStatus_Error, "");
			}
			return;
		}
		if (callback.mResponseStatusCode == 307)
		{
			ProcessErrorXMLClass pc;
			pc.Process(callback.mResponse);
			if (pc.mCode == "TemporaryRedirect")
			{
				if (pc.mEndpoint.empty())
				{
					LogString("PutS3Object: kS3RequestResult_307TemporaryRedirect but new endpoint is empty for host: ", host);
					inCallback.Call(kPutS3ObjectStatus_Error, "");
					return;
				}
				if (pc.mEndpoint == host)
				{
					LogString("PutS3Object: kS3RequestResult_307TemporaryRedirect but new endpoint is the same for host: ", host);
					inCallback.Call(kPutS3ObjectStatus_Error, "");
					return;
				}
				if (++redirectCount >= 5)
				{
					LogString("PutS3Object: too many temporary redirects for host: ", host);
					inCallback.Call(kPutS3ObjectStatus_Error, "");
					return;
				}
				host = pc.mEndpoint;
				continue;
			}
			else
			{
				LogString("PutS3Object: Unparsed 307 Temporary Redirect for host: ", host);
				LogString("-- response: ", callback.mResponse);

				inCallback.Call(kPutS3ObjectStatus_Error, "");
			}
			return;
		}
		if (callback.mResponseStatusCode != 200)
		{
			ProcessErrorXMLClass pc;
			pc.Process(callback.mResponse);
			if (pc.mCode == "AccessDenied")
			{
				Log("PutS3Object: AccessDenied");
				inCallback.Call(kPutS3ObjectStatus_AccessDenied, "");
				return;
			}
			if (pc.mCode == "RequestTimeout")
			{
				Log("PutS3Object: RequestTimeout");
				inCallback.Call(kPutS3ObjectStatus_TimedOut, "");
				return;
			}

			LogString("PutS3Object: responseCode != 200 for URL: ", url);
			LogSInt32("-- responseCode: ", callback.mResponseStatusCode);
			LogString("-- response: ", callback.mResponse);

			if (callback.mResponseStatusCode == 500)
			{
				inCallback.Call(kPutS3ObjectStatus_ServerError, "");
			}
			else
			{
				inCallback.Call(kPutS3ObjectStatus_Error, "");
			}
			return;
		}

		std::string version;
		const StringPairVector& responseParams(callback.mParams);
		StringPairVector::const_iterator end = responseParams.end();
		for (StringPairVector::const_iterator it = responseParams.begin(); it != end; ++it)
		{
			std::string name(it->first);
			if ((name == "X-Amz-Version-Id") ||
				(name == "x-amz-version-id"))
			{
				version = it->second;
				break;
			}
		}

		inCallback.Call(kPutS3ObjectStatus_Success, version);
		break;
	}
}
	
} // namespace s3
} // namespace hermit

#endif


