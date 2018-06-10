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

#ifndef GetS3BucketLocation_h
#define GetS3BucketLocation_h

#include <string>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/HTTP/HTTPSession.h"
#include "S3Result.h"

namespace hermit {
	namespace s3 {
		
		//
		DEFINE_ASYNC_FUNCTION_3A(GetS3BucketLocationCompletion,
								 HermitPtr,
								 S3Result,						// result
								 std::string);					// location
		
		//
		void GetS3BucketLocation(const HermitPtr& h_,
								 const http::HTTPSessionPtr& session,
								 const std::string& bucketName,
								 const std::string& awsPublicKey,
								 const std::string& awsPrivateKey,
								 const GetS3BucketLocationCompletionPtr& completion);
		
	} // namespace s3
} // namespace hermit

#endif 
