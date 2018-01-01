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
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "HTTPParamVector.h"

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
		DEFINE_ASYNC_FUNCTION_3A(HTTPRequestStatusBlock,
								 HermitPtr,
								 int,									// statusCode
								 HTTPParamVector);						// headerParams

		//
		DEFINE_ASYNC_FUNCTION_4A(HTTPRequestResponseBlock,
								 HermitPtr,
								 int,									// statusCode
								 HTTPParamVector,						// headerParams
								 DataBuffer);							// resonseData
		
		//
		DEFINE_ASYNC_FUNCTION_2A(HTTPRequestCompletionBlock,
								 HermitPtr,
								 HTTPRequestResult);
		
		//
		class HTTPRequestStatus : public HTTPRequestStatusBlock {
		public:
			//
			HTTPRequestStatus() : mStatusCode(0) {
			}
			
			//
			virtual void Call(const HermitPtr& h_,
							  const int& statusCode,
							  const HTTPParamVector& headerParams) override {
				mStatusCode = statusCode;
				mHeaderParams = headerParams;
			}
			
			//
			int mStatusCode;
			HTTPParamVector mHeaderParams;
		};
		typedef std::shared_ptr<HTTPRequestStatus> HTTPRequestStatusPtr;

		//
		class HTTPRequestResponse : public HTTPRequestResponseBlock {
		public:
			//
			HTTPRequestResponse() : mStatusCode(0) {
			}
			
			//
			virtual void Call(const HermitPtr& h_,
							  const int& statusCode,
							  const HTTPParamVector& headerParams,
							  const DataBuffer& responseData) override {
				mStatusCode = statusCode;
				mHeaderParams = headerParams;
				if (responseData.second > 0) {
					mResponseData.assign(responseData.first, responseData.second);
				}
			}
			
			//
			int mStatusCode;
			HTTPParamVector mHeaderParams;
			std::string mResponseData;
		};
		typedef std::shared_ptr<HTTPRequestResponse> HTTPRequestResponsePtr;

	} // namespace http
} // namespace hermit

#endif
