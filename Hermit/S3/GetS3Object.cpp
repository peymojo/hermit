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
		
		namespace
		{
			//
			//
			class StreamInDataHandler
			:
			public StreamInS3ObjectDataHandlerFunction
			{
			public:
				//
				//
				StreamInDataHandler()
				{
				}
				
				//
				//
				bool Function(const uint64_t& inExpectedDataSize,
							  const DataBuffer& inDataPart,
							  const bool& inIsEndOfStream)
				{
					if (inDataPart.second > 0)
					{
						mData += std::string(inDataPart.first, inDataPart.second);
					}
					return true;
				}
				
				//
				//
				std::string mData;
			};
			
		} // private namespace
		
		//
		void GetS3Object(const HermitPtr& h_,
						 const std::string& inAWSPublicKey,
						 const std::string& inAWSSigningKey,
						 const uint64_t& inAWSSigningKeySize,
						 const std::string& inAWSRegion,
						 const std::string& inS3BucketName,
						 const std::string& inS3ObjectKey,
						 const GetS3ObjectResponseBlockPtr& inResponseBlock,
						 const S3CompletionBlockPtr& inCompletion) {
			StreamInDataHandler dataHandler;
			auto result = StreamInS3Object(h_,
										   inAWSPublicKey,
										   inAWSSigningKey,
										   inAWSSigningKeySize,
										   inAWSRegion,
										   inS3BucketName,
										   inS3ObjectKey,
										   dataHandler);
			if (result == S3Result::kSuccess) {
				inResponseBlock->Call(DataBuffer(dataHandler.mData.data(), dataHandler.mData.size()));
			}
			inCompletion->Call(h_, result);
		}
		
	} // namespace s3
} // namespace hermit
