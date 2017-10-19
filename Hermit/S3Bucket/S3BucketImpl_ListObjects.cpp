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
#include "Hermit/Foundation/Thread.h"
#include "Hermit/S3/S3ListObjects.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
			namespace {
				
				// ListObjects operation class
				class ListObjectsOp {
				private:
					S3BucketImpl& mBucket;
					std::string mPrefix;
					s3::ObjectKeyReceiver& mReceiver;
					s3::S3Result mResult;
					int mAccessDeniedRetries;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "ListObjects";
					}
					
					//
					ListObjectsOp(S3BucketImpl& bucket, const std::string& prefix, s3::ObjectKeyReceiver& receiver) :
					mBucket(bucket),
					mPrefix(prefix),
					mReceiver(receiver),
					mResult(s3::S3Result::kUnknown),
					mAccessDeniedRetries(0) {
					}
					
					//
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						return S3ListObjects(h_,
											 mBucket.mAWSPublicKey,
											 mBucket.mAWSSigningKey,
											 mBucket.mAWSRegion,
											 mBucket.mBucketName,
											 mPrefix,
											 mReceiver);
					}
					
					//
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
						NOTIFY_ERROR(h_, "ListObjects: error result encountered:", (int)result);
						mResult = result;
					}
					
					//
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "ListObjects: maximum retries exceeded, most recent result:", (int)result);
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
				
			} // private namespace
			
			//
			s3::S3Result S3BucketImpl::ListObjects(const HermitPtr& h_,
												   const std::string& prefix,
												   s3::ObjectKeyReceiver& receiver) {
				ListObjectsOp listObjects(*this, prefix, receiver);
				s3::RetryClassT<ListObjectsOp> retry;
				retry.AttemptWithRetry(h_, listObjects);
				return listObjects.GetResult();
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
