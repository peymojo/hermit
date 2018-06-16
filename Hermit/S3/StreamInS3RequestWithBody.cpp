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
#include "Hermit/HTTP/CreateHTTPSession.h"
#include "Hermit/XML/ParseXMLData.h"
#include "StreamInS3RequestWithBody.h"

namespace hermit {
	namespace s3 {
		namespace StreamInS3RequestWithBody_Impl {

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
			class DataHandler : public DataHandlerBlock {
			public:
				//
				DataHandler(const DataHandlerBlockPtr& dataHandler) :
				mDataHandler(dataHandler) {
				}
				
				//
				virtual StreamDataResult HandleData(const HermitPtr& h_, const DataBuffer& data, bool isEndOfData) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					return mDataHandler->HandleData(h_, data, isEndOfData);
				}
				
				//
				DataHandlerBlockPtr mDataHandler;
				std::string mData;
			};
			typedef std::shared_ptr<DataHandler> DataHandlerPtr;

			//
			class HTTPCompletion : public http::HTTPRequestCompletionBlock {
			public:
				//
				HTTPCompletion(const http::HTTPSessionPtr& httpSession,
							   const std::string& url,
							   const DataHandlerPtr& dataHandler,
							   const http::HTTPRequestStatusPtr& httpStatus,
							   const StreamInS3RequestCompletionPtr& completion) :
				mHTTPSession(httpSession),
				mURL(url),
				mDataHandler(dataHandler),
				mHTTPStatus(httpStatus),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const http::HTTPRequestResult& result) override {
					if (result != http::HTTPRequestResult::kSuccess) {
						if (result == http::HTTPRequestResult::kCanceled) {
							mCompletion->Call(h_, S3Result::kCanceled, S3ParamVector());
							return;
						}
						if (result == http::HTTPRequestResult::kNetworkConnectionLost) {
							mCompletion->Call(h_, S3Result::kNetworkConnectionLost, S3ParamVector());
							return;
						}
						if (result == http::HTTPRequestResult::kNoNetworkConnection) {
							mCompletion->Call(h_, S3Result::kNoNetworkConnection, S3ParamVector());
							return;
						}
						if (result == http::HTTPRequestResult::kTimedOut) {
							mCompletion->Call(h_, S3Result::kTimedOut, S3ParamVector());
							return;
						}
						if (result == http::HTTPRequestResult::kHostNotFound) {
							NOTIFY_ERROR(h_, "StreamInHTTPRequestWithBody returned host not found for:", mURL);
						}
						else {
							NOTIFY_ERROR(h_, "StreamInHTTPRequestWithBody failed for URL:", mURL);
						}
						mCompletion->Call(h_, S3Result::kError, S3ParamVector());
						return;
					}
					if ((mHTTPStatus->mStatusCode < 200) || (mHTTPStatus->mStatusCode >= 300)) {
						if (mHTTPStatus->mStatusCode == 301) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "PermanentRedirect") {
								NOTIFY_ERROR(h_,
											 "301 Permanent Redirect for URL:", mURL,
											 "new endpoint:", pc.mEndpoint);
								http::HTTPParamVector endpointParams;
								endpointParams.push_back(std::make_pair("Endpoint", pc.mEndpoint));
								mCompletion->Call(h_, S3Result::k301PermanentRedirect, endpointParams);
								return;
							}
							NOTIFY_ERROR(h_,
										 "Unparsed 301 Permanent Redirect for URL:", mURL,
										 "response:", mDataHandler->mData);
							mCompletion->Call(h_, S3Result::kError, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 307) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "TemporaryRedirect") {
								http::HTTPParamVector endpointParams;
								endpointParams.push_back(std::make_pair("Endpoint", pc.mEndpoint));
								mCompletion->Call(h_, S3Result::k307TemporaryRedirect, endpointParams);
								return;
							}

							NOTIFY_ERROR(h_,
										 "Unparsed 307 Temporary Redirect for URL:", mURL,
										 "response:", mDataHandler->mData);
							mCompletion->Call(h_, S3Result::kError, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 400) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "RequestTimeout") {
								//	server-side timeout, seen when paused in the debugger so a bit artificial but still
								//	worth having a special path for so we get a retry out of it.
								mCompletion->Call(h_, S3Result::kTimedOut, S3ParamVector());
								return;
							}
							if (pc.mCode == "AuthorizationHeaderMalformed") {
								NOTIFY_ERROR(h_,
											 "S3Result::kAuthorizationHeaderMalformed; response:",
											 mDataHandler->mData);
								mCompletion->Call(h_, S3Result::kAuthorizationHeaderMalformed, S3ParamVector());
								return;
							}
							
							NOTIFY_ERROR(h_, "400 Bad Request, response:", mDataHandler->mData);
							mCompletion->Call(h_, S3Result::k400BadRequest, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 403) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "SignatureDoesNotMatch") {
								NOTIFY_ERROR(h_, "403 - SignatureDoesNotMatch");
							}
							else {
								NOTIFY_ERROR(h_, "403, response:", mDataHandler->mData);
							}
							mCompletion->Call(h_, S3Result::k403AccessDenied, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 404) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "NoSuchBucket") {
								mCompletion->Call(h_, S3Result::k404NoSuchBucket, S3ParamVector());
								return;
							}
							mCompletion->Call(h_, S3Result::k404EntityNotFound, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 500) {
							ProcessXMLClass pc(h_);
							pc.Process(mDataHandler->mData);
							if (pc.mCode == "InternalError") {
								mCompletion->Call(h_, S3Result::kS3InternalError, S3ParamVector());
								return;
							}
							NOTIFY_ERROR(h_,
										 "Non-standard 500 error for URL:", mURL,
										 "response:", mDataHandler->mData);
							mCompletion->Call(h_, S3Result::k500InternalServerError, S3ParamVector());
							return;
						}
						if (mHTTPStatus->mStatusCode == 503) {
							NOTIFY_ERROR(h_, "503 Service Unavailable for URL:", mURL);
							mCompletion->Call(h_, S3Result::k503ServiceUnavailable, S3ParamVector());
							return;
						}
						
						NOTIFY_ERROR(h_,
									 "Unexpected responseCode for URL:", mURL,
									 "status code:", mHTTPStatus->mStatusCode,
									 "response:", mDataHandler->mData);
							
						mCompletion->Call(h_, S3Result::kError, S3ParamVector());
						return;
					}

					mCompletion->Call(h_, S3Result::kSuccess, mHTTPStatus->mHeaderParams);
				}
				
				//
				http::HTTPSessionPtr mHTTPSession;
				std::string mURL;
				DataHandlerPtr mDataHandler;
				http::HTTPRequestStatusPtr mHTTPStatus;
				StreamInS3RequestCompletionPtr mCompletion;
			};
			
		} // namespace StreamInS3RequestWithBody_Impl
		using namespace StreamInS3RequestWithBody_Impl;
		
		//
		void StreamInS3RequestWithBody(const HermitPtr& h_,
									   const http::HTTPSessionPtr& session,
									   const std::string& url,
									   const std::string& method,
									   const S3ParamVector& params,
									   const SharedBufferPtr& body,
									   const DataHandlerBlockPtr& dataHandler,
									   const StreamInS3RequestCompletionPtr& completion) {
			auto dataHandlerProxy = std::make_shared<DataHandler>(dataHandler);
			auto status = std::make_shared<http::HTTPRequestStatus>();
			auto httpCompletion = std::make_shared<HTTPCompletion>(session,
																   url,
																   dataHandlerProxy,
																   status,
																   completion);
			
			session->StreamInRequestWithBody(h_,
											 url,
											 method,
											 params,
											 body,
											 dataHandlerProxy,
											 status,
											 httpCompletion);
		}
		
	} // namespace s3
} // namespace hermit
