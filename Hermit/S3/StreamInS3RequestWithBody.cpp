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
#include "Hermit/HTTP/StreamInHTTPRequestWithBody.h"
#include "Hermit/XML/ParseXMLData.h"
#include "StreamInS3RequestWithBody.h"

namespace hermit {
	namespace s3 {
		
		namespace
		{
			//
			//
			class ProcessXMLClass : xml::ParseXMLClient
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
					kParseState_IgnoredElement
				};
				
				//
				//
				typedef std::stack<ParseState> ParseStateStack;
				
			public:
				//
				//
				ProcessXMLClass(const HermitPtr& h_)
				:
				mH_(h_),
				mParseState(kParseState_New)
				{
				}
								
				//
				xml::ParseXMLStatus Process(const std::string& inXMLData) {
					return xml::ParseXMLData(mH_, inXMLData, *this);
				}
				
				//
				virtual xml::ParseXMLStatus OnStart(const std::string& inStartTag,
													const std::string& inAttributes,
													bool inIsEmptyElement) override {
					if (mParseState == kParseState_New)
					{
						if (inStartTag == "Error")
						{
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
							PushState(kParseState_Code);
						}
						else if (inStartTag == "Endpoint")
						{
							PushState(kParseState_Endpoint);
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
					if (mParseState == kParseState_Code)
					{
						mCode = inContent;
					}
					else if (mParseState == kParseState_Endpoint)
					{
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
				HermitPtr mH_;
				ParseState mParseState;
				ParseStateStack mParseStateStack;
				std::string mCode;
				std::string mEndpoint;
			};
			
			//
			//
			class StreamInDataHandler
			:
			public http::StreamInHTTPRequestDataHandlerFunction
			{
			public:
				//
				StreamInDataHandler(const StreamInS3RequestDataHandlerFunctionRef& inDataHandlerFunction) :
				mDataHandlerFunction(inDataHandlerFunction) {
				}
				
				//
				bool Function(const uint64_t& inExpectedDataSize,
							  const DataBuffer& inDataPart,
							  const bool& inIsEndOfStream) {
					if (inDataPart.second > 0) {
						mResponse.append(inDataPart.first, inDataPart.second);
					}
					if (!mDataHandlerFunction.Call(inExpectedDataSize, inDataPart, inIsEndOfStream)) {
						return false;
					}
					return true;
				}
				
				//
				const StreamInS3RequestDataHandlerFunctionRef& mDataHandlerFunction;
				std::string mResponse;
			};
			
			//
			typedef std::pair<std::string, std::string> HTTPParam;
			typedef std::vector<HTTPParam> HTTPParamVector;
			
		} // private namespace
		
		//
		void StreamInS3RequestWithBody(const HermitPtr& h_,
									   const std::string& inURL,
									   const std::string& inMethod,
									   const EnumerateStringValuesFunctionRef& inParamsFunction,
									   const DataBuffer& inBodyData,
									   const StreamInS3RequestDataHandlerFunctionRef& inDataHandlerFunction,
									   const StreamInS3RequestCallbackRef& inCallback) {
			EnumerateStringValuesCallbackClass paramCallback;
			if (!inParamsFunction.Call(paramCallback)) {
				NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: inParamsFunction returned false for URL:", inURL);
				inCallback.Call(S3Result::kError, nullptr);
				return;
			}
			
			EnumerateStringValuesFunctionClass headerParams(paramCallback.mValues);
			StreamInDataHandler dataHandler(inDataHandlerFunction);
			http::StreamInHTTPRequestCallbackClassT<HTTPParamVector> result;
			http::StreamInHTTPRequestWithBody(h_,
											  inURL,
											  inMethod,
											  headerParams,
											  inBodyData,
											  dataHandler,
											  result);
			
			if (result.mResult != http::HTTPRequestResult::kSuccess) {
				if (result.mResult == http::HTTPRequestResult::kCanceled) {
					inCallback.Call(S3Result::kCanceled, nullptr);
				}
				else if (result.mResult == http::HTTPRequestResult::kNetworkConnectionLost) {
					inCallback.Call(S3Result::kNetworkConnectionLost, nullptr);
				}
				else if (result.mResult == http::HTTPRequestResult::kNoNetworkConnection) {
					inCallback.Call(S3Result::kNoNetworkConnection, nullptr);
				}
				else if (result.mResult == http::HTTPRequestResult::kTimedOut) {
					inCallback.Call(S3Result::kTimedOut, nullptr);
				}
				else {
					if (result.mResult == http::HTTPRequestResult::kHostNotFound) {
						NOTIFY_ERROR(h_,
									 "StreamInS3RequestWithBody: StreamInHTTPRequestWithBody returned host not found for:",
									 inURL);
					}
					else {
						NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: StreamInHTTPRequestWithBody failed for URL:", inURL);
					}
					
					inCallback.Call(S3Result::kError, nullptr);
				}
			}
			else if ((result.mResponseStatusCode < 200) || (result.mResponseStatusCode >= 300)) {
				if (result.mResponseStatusCode == 301) {
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "PermanentRedirect") {
						NOTIFY_ERROR(h_,
									 "StreamInS3RequestWithBody: 301 Permanent Redirect for URL:", inURL,
									 "new endpoint:", pc.mEndpoint);
						
						HTTPParamVector endpointParams;
						endpointParams.push_back(HTTPParam("Endpoint", pc.mEndpoint));
						EnumerateStringValuesFunctionClass resultParams(endpointParams);
						inCallback.Call(S3Result::k301PermanentRedirect, resultParams);
					}
					else {
						NOTIFY_ERROR(h_,
									 "StreamInS3RequestWithBody: Unparsed 301 Permanent Redirect for URL:", inURL,
									 "response:", dataHandler.mResponse);
						
						inCallback.Call(S3Result::kError, nullptr);
					}
				}
				else if (result.mResponseStatusCode == 307) {
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "TemporaryRedirect") {
						HTTPParamVector endpointParams;
						endpointParams.push_back(HTTPParam("Endpoint", pc.mEndpoint));
						EnumerateStringValuesFunctionClass resultParams(endpointParams);
						inCallback.Call(S3Result::k307TemporaryRedirect, resultParams);
					}
					else {
						NOTIFY_ERROR(h_,
									 "StreamInS3RequestWithBody: Unparsed 307 Temporary Redirect for URL:", inURL,
									 "response:", dataHandler.mResponse);
						
						inCallback.Call(S3Result::kError, nullptr);
					}
				}
				else if (result.mResponseStatusCode == 400) {
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "RequestTimeout") {
						//	server-side timeout, seen when paused in the debugger so a bit artificial but still
						//	worth having a special path for so we get a retry out of it.
						inCallback.Call(S3Result::kTimedOut, nullptr);
						return;
					}
					if (pc.mCode == "AuthorizationHeaderMalformed") {
						NOTIFY_ERROR(h_,
									 "StreamInS3RequestWithBody: S3Result::kAuthorizationHeaderMalformed; response:",
									 dataHandler.mResponse);
						
						inCallback.Call(S3Result::kAuthorizationHeaderMalformed, nullptr);
						return;
					}
					
					NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: 400 Bad Request; response:", dataHandler.mResponse);
					
					inCallback.Call(S3Result::k400BadRequest, nullptr);
				}
				else if (result.mResponseStatusCode == 403) {
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "SignatureDoesNotMatch") {
						NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: 403 - SignatureDoesNotMatch");
					}
					else {
						NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: 403, response:", dataHandler.mResponse);
					}
					inCallback.Call(S3Result::k403AccessDenied, nullptr);
				}
				else if (result.mResponseStatusCode == 404) {
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "NoSuchBucket") {
						inCallback.Call(S3Result::k404NoSuchBucket, nullptr);
						return;
					}
					inCallback.Call(S3Result::k404EntityNotFound, nullptr);
				}
				else if (result.mResponseStatusCode == 500)
				{
					ProcessXMLClass pc(h_);
					pc.Process(dataHandler.mResponse);
					if (pc.mCode == "InternalError")
					{
						inCallback.Call(S3Result::kS3InternalError, nullptr);
						return;
					}
					NOTIFY_ERROR(h_,
								 "StreamInS3RequestWithBody: Non-standard 500 error for URL:", inURL,
								 "response:", dataHandler.mResponse);
					
					inCallback.Call(S3Result::k500InternalServerError, nullptr);
				}
				else if (result.mResponseStatusCode == 503)
				{
					NOTIFY_ERROR(h_, "StreamInS3RequestWithBody: 503 Service Unavailable for URL:", inURL);
					inCallback.Call(S3Result::k503ServiceUnavailable, nullptr);
				}
				else
				{
					NOTIFY_ERROR(h_,
								 "StreamInS3RequestWithBody: unexpected responseCode for URL:", inURL,
								 "responseCode:", result.mResponseStatusCode,
								 "response:", dataHandler.mResponse);
					
					inCallback.Call(S3Result::kError, nullptr);
				}
			}
			else {
				EnumerateStringValuesFunctionClass resultParams(result.mParams);
				inCallback.Call(S3Result::kSuccess, resultParams);
			}
		}
		
	} // namespace s3
} // namespace hermit
