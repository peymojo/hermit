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
#include "StreamInS3Object.h"
#include "GetS3Object.h"

namespace hermit {
	namespace s3 {
		namespace GetS3Object_Impl {

			//
			class Receiver : public DataReceiver {
			public:
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<Receiver> ReceiverPtr;

			//
			class StreamCompletion : public S3CompletionBlock {
			public:
				//
				StreamCompletion(const ReceiverPtr& dataReceiver,
								 const GetS3ObjectResponseBlockPtr& response,
								 const S3CompletionBlockPtr& completion) :
				mDataReceiver(dataReceiver),
				mResponse(response),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const S3Result& result) override {
					if (result == S3Result::kSuccess) {
						mResponse->Call(DataBuffer(mDataReceiver->mData.data(), mDataReceiver->mData.size()));
					}
					mCompletion->Call(h_, result);
				}
				
				//
				ReceiverPtr mDataReceiver;
				GetS3ObjectResponseBlockPtr mResponse;
				S3CompletionBlockPtr mCompletion;
			};
			
		} // namespace GetS3Object_Impl
		using namespace GetS3Object_Impl;
		
		//
		void GetS3Object(const HermitPtr& h_,
						 const http::HTTPSessionPtr& session,
						 const std::string& awsPublicKey,
						 const std::string& awsSigningKey,
						 const std::string& awsRegion,
						 const std::string& s3BucketName,
						 const std::string& s3ObjectKey,
						 const GetS3ObjectResponseBlockPtr& response,
						 const S3CompletionBlockPtr& completion) {
			auto dataReceiver = std::make_shared<Receiver>();
			auto streamCompletion = std::make_shared<StreamCompletion>(dataReceiver, response, completion);
			StreamInS3Object(h_,
							 session,
							 awsPublicKey,
							 awsSigningKey,
							 awsRegion,
							 s3BucketName,
							 s3ObjectKey,
							 dataReceiver,
							 streamCompletion);
		}
		
	} // namespace s3
} // namespace hermit
