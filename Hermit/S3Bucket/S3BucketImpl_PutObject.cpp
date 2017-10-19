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
#include "Hermit/S3/PutS3Object.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"
#include "PutMultipartObjectToS3Bucket.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
			namespace {
				
				// above this size we'll use the s3 multipart object api
				const uint64_t kMultipartObjectSizeThreshold = 10 * 1024 * 1024;
				
				// PutObject operation class
				class PutObjectOp {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					DataBuffer mData;
					bool mUseReducedRedundancyStorage;
					s3::PutS3ObjectCallbackRef mCallback;
					int mAccessDeniedRetries;
					std::string mVersion;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "PutObject";
					}
					
					//
					PutObjectOp(
								S3BucketImpl& bucket,
								const std::string& objectKey,
								const DataBuffer& data,
								const bool& useReducedRedundancyStorage,
								const s3::PutS3ObjectCallbackRef& callback)
					:
					mBucket(bucket),
					mObjectKey(objectKey),
					mData(data),
					mUseReducedRedundancyStorage(useReducedRedundancyStorage),
					mCallback(callback),
					mAccessDeniedRetries(0) {
					}
					
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						s3::PutS3ObjectCallbackClass callback;
						PutS3Object(h_,
									mBucket.mAWSPublicKey,
									mBucket.mAWSSigningKey,
									mBucket.mAWSRegion,
									mBucket.mBucketName,
									mObjectKey,
									mData,
									mUseReducedRedundancyStorage,
									callback);
						
						if (callback.mResult == s3::S3Result::kSuccess) {
							mVersion = callback.mVersion;
						}
						return callback.mResult;
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
					
					void ProcessResult(const HermitPtr& h_, const ResultType& result) {
						
						if (result == s3::S3Result::kCanceled) {
							mCallback.Call(result, "");
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mCallback.Call(result, "");
							return;
						}
						NOTIFY_ERROR(h_, "PutObjectToS3Bucket: error result encountered:", (int)result);
						mCallback.Call(result, "");
					}
					
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "PutObjectToS3Bucket: maximum retries exceeded, most recent result:", (int)result);
						mCallback.Call(result, "");
					}
					
					void Canceled() {
						mCallback.Call(s3::S3Result::kCanceled, "");
					}
				};
				
			} // namespace
			
			//
			void S3BucketImpl::PutObject(const HermitPtr& h_,
										 const std::string& inS3ObjectKey,
										 const DataBuffer& inData,
										 const bool& inUseReducedRedundancyStorage,
										 const s3::PutS3ObjectCallbackRef& inCallback) {
				if (inData.second > kMultipartObjectSizeThreshold) {
					PutMultipartObjectToS3Bucket(h_,
												 *this,
												 inS3ObjectKey,
												 inData,
												 inUseReducedRedundancyStorage,
												 inCallback);
					
					return;
				}
				
				PutObjectOp get(*this, inS3ObjectKey, inData, inUseReducedRedundancyStorage, inCallback);
				s3::RetryClassT<PutObjectOp> retry;
				retry.AttemptWithRetry(h_, get);
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
