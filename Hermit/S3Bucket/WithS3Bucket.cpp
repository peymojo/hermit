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

#include "Hermit/Foundation/Notification.h"
#include "S3BucketImpl.h"
#include "WithS3Bucket.h"

namespace hermit {
	namespace s3bucket {
		
		//
		void WithS3Bucket(const HermitPtr& h_,
						  const std::string& inBucketName,
						  const std::string& inAWSPublicKey,
						  const std::string& inAWSPrivateKey,
						  const WithS3BucketCallbackRef& inCallback) {
			if (inBucketName.empty()) {
				NOTIFY_ERROR(h_, "WithS3Bucket: inBucketName is empty.");
				inCallback.Call(WithS3BucketStatus::kError, 0);
				return;
			}
			
			auto bucket = std::make_shared<impl::S3BucketImpl>(inAWSPublicKey, inAWSPrivateKey, inBucketName);
			
			WithS3BucketStatus status = bucket->Init(h_);
			if (status == WithS3BucketStatus::kCancel) {
				inCallback.Call(WithS3BucketStatus::kCancel, nullptr);
				return;
			}
			if (status != WithS3BucketStatus::kSuccess) {
				if (status == WithS3BucketStatus::kError) {
					NOTIFY_ERROR(h_, "WithS3Bucket: bucket->Init returned WithS3BucketStatus::kError.");
				}
				bucket = nullptr;
			}
			inCallback.Call(status, bucket);
		}
		
	} // namespace s3bucket
} // namespace hermit
