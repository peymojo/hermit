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

#ifndef StreamInS3Object_h
#define StreamInS3Object_h

#include <string>
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"
#include "S3Result.h"

namespace hermit {
	namespace s3 {
		
		//
		DEFINE_CALLBACK_3A(StreamInS3ObjectDataHandlerFunction,
						   uint64_t,					// inExpectedDataSize
						   DataBuffer,					// inDataPart
						   bool);						// inIsEndOfStream
		
		//
		S3Result StreamInS3Object(const HermitPtr& h_,
								  const std::string& inAWSPublicKey,
								  const std::string& inAWSSigningKey,
								  const uint64_t& inAWSSigningKeySize,
								  const std::string& inAWSRegion,
								  const std::string& inS3BucketName,
								  const std::string& inS3ObjectKey,
								  const StreamInS3ObjectDataHandlerFunctionRef& inDataHandlerFunction);
		
	} // namespace s3
} // namespace hermit

#endif