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
		
		namespace SendS3CommandWithData_Impl {

			//
			class DataHandler : public DataHandlerBlock {
			public:
				//
				virtual void HandleData(const HermitPtr& h_,
										const DataBuffer& data,
										bool isEndOfData,
										const StreamResultBlockPtr& resultBlock) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					resultBlock->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<DataHandler> DataHandlerPtr;
			
			//
			class StreamInCompletion : public StreamInS3RequestCompletion {
			public:
				//
				StreamInCompletion(const DataHandlerPtr& dataHandler, const SendS3CommandCompletionPtr& completion) :
				mDataHandler(dataHandler),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const S3Result& result, const S3ParamVector& params) override {
					mCompletion->Call(h_,
									  result,
									  params,
									  DataBuffer(mDataHandler->mData.data(), mDataHandler->mData.size()));
				}
				
				//
				DataHandlerPtr mDataHandler;
				SendS3CommandCompletionPtr mCompletion;
			};
			
		} // namespace SendS3CommandWithData_Impl
		using namespace SendS3CommandWithData_Impl;
		
		//
		void SendS3CommandWithData(const HermitPtr& h_,
								   const std::string& url,
								   const std::string& method,
								   const S3ParamVector& params,
								   const SharedBufferPtr& data,
								   const SendS3CommandCompletionPtr& completion) {
			auto dataHandler = std::make_shared<DataHandler>();
			auto streamCompletion = std::make_shared<StreamInCompletion>(dataHandler, completion);
			StreamInS3RequestWithBody(h_,
									  url,
									  method,
									  params,
									  data,
									  dataHandler,
									  streamCompletion);
		}
		
	} // namespace s3
} // namespace hermit
