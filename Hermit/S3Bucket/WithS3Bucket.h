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

#include "Hermit/Foundation/Callback.h"
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
		DEFINE_CALLBACK_2A(
						   WithS3BucketCallback,
						   WithS3BucketStatus,
						   S3BucketPtr);
		
		//
		class WithS3BucketCallbackClass
		:
		public WithS3BucketCallback {
		public:
			//
			WithS3BucketCallbackClass()
			:
			mStatus(WithS3BucketStatus::kUnknown) {
			}
			
			//
			bool Function(const WithS3BucketStatus& inStatus, const S3BucketPtr& inS3Bucket) {
				mStatus = inStatus;
				if (inStatus == WithS3BucketStatus::kSuccess) {
					mS3Bucket = inS3Bucket;
				}
				return true;
			}
			
			//
			WithS3BucketStatus mStatus;
			S3BucketPtr mS3Bucket;
		};
		
		//
		void WithS3Bucket(const HermitPtr& h_,
						  const std::string& inBucketName,
						  const std::string& inAWSPublicKey,
						  const std::string& inAWSPrivateKey,
						  const WithS3BucketCallbackRef& inCallback);
		
	} // namespace s3bucket
} // namespace hermit

#endif
