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

#ifndef HTTPSession_h
#define HTTPSession_h

#include <memory>
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/SharedBuffer.h"
#include "Hermit/Foundation/StreamDataFunction.h"
#include "HTTPRequestResult.h"

namespace hermit {
	namespace http {
		
		//
		class HTTPSession {
		public:
			//
			virtual void SendRequest(const HermitPtr& h_,
									 const std::string& url,
									 const std::string& method,
									 const HTTPParamVector& headerParams,
									 const HTTPRequestResponseBlockPtr& response,
									 const HTTPRequestCompletionBlockPtr& completion);
			
			//
			virtual void SendRequestWithBody(const HermitPtr& h_,
											 const std::string& url,
											 const std::string& method,
											 const HTTPParamVector& headerParams,
											 const SharedBufferPtr& body,
											 const HTTPRequestResponseBlockPtr& response,
											 const HTTPRequestCompletionBlockPtr& completion);

			//
			virtual void StreamInRequest(const HermitPtr& h_,
										 const std::string& url,
										 const std::string& method,
										 const HTTPParamVector& headerParams,
										 const DataHandlerBlockPtr& dataHandler,
										 const HTTPRequestStatusBlockPtr& status,
										 const HTTPRequestCompletionBlockPtr& completion);
			
			//
			virtual void StreamInRequestWithBody(const HermitPtr& h_,
												 const std::string& url,
												 const std::string& method,
												 const HTTPParamVector& headerParams,
												 const SharedBufferPtr& body,
												 const DataHandlerBlockPtr& dataHandler,
												 const HTTPRequestStatusBlockPtr& status,
												 const HTTPRequestCompletionBlockPtr& completion);
			
		protected:
			//
			virtual ~HTTPSession() = default;
		};
		typedef std::shared_ptr<HTTPSession> HTTPSessionPtr;

	} // namespace http
} // namespace hermit

#endif /* HTTPSession_h */
