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
#include "Hermit/S3/S3DeleteObject.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
			namespace {
				
				// DeleteObject operation class
				class DeleteObjectOp {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					std::string mVersion;
					s3::S3Result mResult;
					int mAccessDeniedRetries;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "DeleteObject";
					}
					
					//
					DeleteObjectOp(S3BucketImpl& bucket, const std::string& objectKey)
					:
					mBucket(bucket),
					mObjectKey(objectKey),
					mResult(s3::S3Result::kUnknown),
					mAccessDeniedRetries(0) {
					}
					
					//
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						return s3::S3DeleteObject(h_,
												  mBucket.mAWSPublicKey,
												  mBucket.mAWSSigningKey,
												  mBucket.mAWSRegion,
												  mBucket.mBucketName,
												  mObjectKey);
					}
					
					bool ShouldRetry(const ResultType& result) {
						if ((result == s3::S3Result::kTimedOut) ||
							(result == s3::S3Result::kNetworkConnectionLost) ||
							(result == s3::S3Result::kS3InternalError) ||
							(result == s3::S3Result::k500InternalServerError) ||
							(result == s3::S3Result::k503ServiceUnavailable) ||
							// borderline candidate for retry, but I've seen it recover "in the wild":
							(result == s3::S3Result::kHostNotFound)) {
							return true;
						}
						// we allow a single retry on PermissionDenied since i've seen this fail due to
						// flaky network behavior in the wild. (but we don't want to spam the server in
						// cases where access is indeed denied so we only do it once.)
						if ((result == s3::S3Result::k403AccessDenied) && (mAccessDeniedRetries == 0)) {
							++mAccessDeniedRetries;
							return true;
						}
						return false;
					}
					
					//
					void ProcessResult(const HermitPtr& h_, const ResultType& result) {
						
						if (result == s3::S3Result::kCanceled) {
							mResult = result;
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mResult = result;
							return;
						}
						NOTIFY_ERROR(h_, "DeleteObject: error result encountered:", (int)result);
						mResult = result;
					}
					
					//
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "DeleteObject: maximum retries exceeded, most recent result:", (int)result);
						mResult = result;
					}
					
					//
					void Canceled() {
						mResult = s3::S3Result::kCanceled;
					}
					
					//
					s3::S3Result GetResult() const {
						return mResult;
					}
				};
				
			} // namespace
			
			//
			s3::S3Result S3BucketImpl::DeleteObject(const HermitPtr& h_, const std::string& inObjectKey) {
				DeleteObjectOp deleteObject(*this, inObjectKey);
				s3::RetryClassT<DeleteObjectOp> retry;
				retry.AttemptWithRetry(h_, deleteObject);
				return deleteObject.GetResult();
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
