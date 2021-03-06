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

#include "Hermit/Foundation/Notification.h"
#include "HTTPSession.h"

namespace hermit {
	namespace http {
		
		//
		void HTTPSession::SendRequest(const HermitPtr& h_,
									  const std::string& url,
									  const std::string& method,
									  const HTTPParamVector& headerParams,
									  const HTTPRequestResponseBlockPtr& response,
									  const HTTPRequestCompletionBlockPtr& completion) {
			NOTIFY_ERROR(h_, "HTTPSession::SendRequest unimplemented");
			completion->Call(h_, HTTPRequestResult::kError);
		}

		//
		void HTTPSession::SendRequestWithBody(const HermitPtr& h_,
											  const std::string& url,
											  const std::string& method,
											  const HTTPParamVector& headerParams,
											  const SharedBufferPtr& body,
											  const HTTPRequestResponseBlockPtr& response,
											  const HTTPRequestCompletionBlockPtr& completion) {
			NOTIFY_ERROR(h_, "HTTPSession::SendRequestWithBody unimplemented");
			completion->Call(h_, HTTPRequestResult::kError);
		}
		
		//
		void HTTPSession::StreamInRequest(const HermitPtr& h_,
										  const std::string& url,
										  const std::string& method,
										  const HTTPParamVector& headerParams,
										  const DataReceiverPtr& dataReceiver,
										  const HTTPRequestStatusBlockPtr& status,
										  const HTTPRequestCompletionBlockPtr& completion) {
			NOTIFY_ERROR(h_, "HTTPSession::StreamInRequest unimplemented");
			completion->Call(h_, HTTPRequestResult::kError);
		}
		
		//
		void HTTPSession::StreamInRequestWithBody(const HermitPtr& h_,
												  const std::string& url,
												  const std::string& method,
												  const HTTPParamVector& headerParams,
												  const SharedBufferPtr& body,
												  const DataReceiverPtr& dataReceiver,
												  const HTTPRequestStatusBlockPtr& status,
												  const HTTPRequestCompletionBlockPtr& completion) {
			NOTIFY_ERROR(h_, "HTTPSession::StreamInRequestWithBody unimplemented");
			completion->Call(h_, HTTPRequestResult::kError);
		}
		
	} // namespace http
} // namespace hermit

