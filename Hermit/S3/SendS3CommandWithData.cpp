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
#include "StreamInS3RequestWithBody.h"
#include "SendS3CommandWithData.h"

namespace hermit {
	namespace s3 {
		
		namespace
		{
			//
			//
			class StreamInDataHandler
			:
			public StreamInS3RequestDataHandlerFunction
			{
			public:
				//
				//
				StreamInDataHandler()
				{
				}
				
				//
				//
				bool Function(
							  const uint64_t& inExpectedDataSize,
							  const DataBuffer& inDataPart,
							  const bool& inIsEndOfStream)
				{
					if (inDataPart.second > 0)
					{
						mResponse += std::string(inDataPart.first, inDataPart.second);
					}
					return true;
				}
				
				//
				//
				std::string mResponse;
			};
			
			//
			//
			class StreamInCallback
			:
			public StreamInS3RequestCallback
			{
			public:
				//
				//
				StreamInCallback(
								 const StreamInDataHandler& inDataHandler,
								 const SendS3CommandCallbackRef& inCallback)
				:
				mDataHandler(inDataHandler),
				mCallback(inCallback)
				{
				}
				
				//
				//
				bool Function(
							  const S3Result& inResult,
							  const EnumerateStringValuesFunctionRef& inParamFunction)
				{
					return mCallback.Call(
										  inResult,
										  inParamFunction,
										  DataBuffer(mDataHandler.mResponse.data(), mDataHandler.mResponse.size()));
				}
				
				//
				//
				const StreamInDataHandler& mDataHandler;
				const SendS3CommandCallbackRef& mCallback;
			};
			
		} // private namespace
		
		//
		//
		void SendS3CommandWithData(const HermitPtr& h_,
								   const std::string& inURL,
								   const std::string& inMethod,
								   const EnumerateStringValuesFunctionRef& inParamsFunction,
								   const DataBuffer& inData,
								   const SendS3CommandCallbackRef& inCallback)
		{
			StreamInDataHandler dataHandler;
			StreamInCallback callback(dataHandler, inCallback);
			StreamInS3RequestWithBody(h_,
									  inURL,
									  inMethod,
									  inParamsFunction,
									  inData,
									  dataHandler,
									  callback);
		}
		
	} // namespace s3
} // namespace hermit
