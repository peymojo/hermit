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

#ifndef WithS3Bucket_h
#define WithS3Bucket_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "S3Bucket.h"

namespace hermit {
	namespace s3bucket {
		
		//
		enum class WithS3BucketStatus {
			kUnknown,
			kSuccess,
			kInvalidAccessKey,
			kSignatureDoesNotMatch,
			kAccessDenied,
			kNoSuchBucket,
			kNoNetworkConnection,
			kNetworkConnectionLost,
			kTimedOut,
			kCancel,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(WithS3BucketCompletion, HermitPtr, WithS3BucketStatus, S3BucketPtr);
				
		//
		void WithS3Bucket(const HermitPtr& h_,
						  const std::string& bucketName,
						  const std::string& awsPublicKey,
						  const std::string& awsPrivateKey,
						  const WithS3BucketCompletionPtr& completion);
		
	} // namespace s3bucket
} // namespace hermit

#endif
