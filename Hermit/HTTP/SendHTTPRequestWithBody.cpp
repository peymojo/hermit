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
#include "StreamInHTTPRequestWithBody.h"
#include "SendHTTPRequestWithBody.h"

namespace hermit {
	namespace http {
		
		namespace {

			//
			typedef std::pair<std::string, std::string> StringPair;
			typedef std::vector<StringPair> StringPairVector;
			
			//
			class ResponseParams : public EnumerateStringValuesFunction {
			public:
				//
				ResponseParams(const StringPairVector& inParams) : mParams(inParams) {
				}
				
				//
				bool Function(const EnumerateStringValuesCallbackRef& inCallback) {
					auto end = mParams.end();
					for (auto it = mParams.begin(); it != end; ++it) {
						if (!inCallback.Call(it->first.c_str(), it->second.c_str())) {
							return false;
						}
					}
					return true;
				}
				
			private:
				//
				const StringPairVector& mParams;
			};
			
			//
			class StreamInDataHandler : public StreamInHTTPRequestDataHandlerFunction {
			public:
				//
				StreamInDataHandler() {
				}
				
				//
				bool Function(const uint64_t& inExpectedDataSize, const DataBuffer& inDataPart, const bool& inIsEndOfStream) {
					if (inDataPart.second > 0) {
						mResponse.append(inDataPart.first, inDataPart.second);
					}
					return true;
				}
				
				//
				std::string mResponse;
			};
			
		} // private namespace
		
		//
		void SendHTTPRequestWithBody(const HermitPtr& h_,
									 const std::string& inURL,
									 const std::string& inMethod,
									 const EnumerateStringValuesFunctionRef& inHeaderParamsFunction,
									 const DataBuffer& inBody,
									 const SendHTTPRequestResponseBlockPtr& inResponseBlock,
									 const SendHTTPRequestCompletionBlockPtr& inCompletion) {
			StreamInDataHandler dataHandler;
			StreamInHTTPRequestCallbackClassT<StringPairVector> callback;
			StreamInHTTPRequestWithBody(h_,
										inURL,
										inMethod,
										inHeaderParamsFunction,
										inBody,
										dataHandler,
										callback);
			
			if (callback.mResult == HTTPRequestResult::kSuccess) {
				DataBuffer responseData(dataHandler.mResponse.data(), dataHandler.mResponse.size());
				inResponseBlock->Call(callback.mResponseStatusCode, callback.mParams, responseData);
			}
			inCompletion->Call(h_, callback.mResult);
		}
		
	} // namespace http
} // namespace hermit
