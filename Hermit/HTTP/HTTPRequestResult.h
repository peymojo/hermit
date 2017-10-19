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

#ifndef HTTPRequestResult_h
#define HTTPRequestResult_h

#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/DataBuffer.h"

namespace hermit {
	namespace http {
		
		//
		enum class HTTPRequestResult {
			kUnknown,
			kSuccess,						//	Response received, NOT necessarily status code 2xx
			kCanceled,
			kHostNotFound,
			kNetworkConnectionLost,
			kNoNetworkConnection,
			kTimedOut,
			kError,
		};
		
		//
		typedef std::vector<std::pair<std::string, std::string>> ParamVector;
		
		//
		DEFINE_ASYNC_FUNCTION_3A(SendHTTPRequestResponseBlock,
								 int,									// statusCode
								 ParamVector,							// headerParams
								 DataBuffer);							// resonseData
		
		//
		DEFINE_ASYNC_FUNCTION_2A(SendHTTPRequestCompletionBlock, HermitPtr, HTTPRequestResult);
		
		//
		class SendHTTPRequestResponse : public SendHTTPRequestResponseBlock {
		public:
			//
			SendHTTPRequestResponse() : mStatusCode(0) {
			}
			
			//
			virtual void Call(const int& statusCode, const ParamVector& headerParams, const DataBuffer& responseData) override {
				mStatusCode = statusCode;
				mHeaderParams = headerParams;
				if (responseData.second > 0) {
					mResponseData.assign(responseData.first, responseData.second);
				}
			}
			
			//
			int mStatusCode;
			ParamVector mHeaderParams;
			std::string mResponseData;
		};
		typedef std::shared_ptr<SendHTTPRequestResponse> SendHTTPRequestResponsePtr;

	} // namespace http
} // namespace hermit

#endif
