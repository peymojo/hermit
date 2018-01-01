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

#ifndef S3DeleteObjects_h
#define S3DeleteObjects_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "S3Result.h"

namespace hermit {
	namespace s3 {
		
		//
		DEFINE_ASYNC_FUNCTION_4A(S3DeletedObjectReporter,
								 HermitPtr,
								 std::string,					// inBucketName
								 std::string,					// inObjectKey
								 std::string);					// inObjectVersion
		
		//
		void S3DeleteObjects(const HermitPtr& h_,
							 const std::string& awsPublicKey,
							 const std::string& awsSigningKey,
							 const std::string& awsRegion,
							 const std::string& bucketName,
							 const std::string& pathPrefix,
							 const S3DeletedObjectReporterPtr& deletedObjectReporter,
							 const S3CompletionBlockPtr& completion);
		
	} // namespace s3
} // namespace hermit

#endif
