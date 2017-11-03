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
#include "Hermit/S3/GetS3BucketLocation.h"
#include "Hermit/S3/GetS3ObjectWithVersion.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
			namespace {
				
				//
				typedef std::shared_ptr<S3BucketImpl> S3BucketImplPtr;
				
				//
				class GetObjectVersionClass;
				typedef std::shared_ptr<GetObjectVersionClass> GetObjectVersionClassPtr;
				
				//
				class GetObjectVersionCompletion : public s3::S3CompletionBlock {
				public:
					//
					GetObjectVersionCompletion(const GetObjectVersionClassPtr& getObjectVersionClass) :
					mGetObjectVersionClass(getObjectVersionClass) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override;
					
					//
					GetObjectVersionClassPtr mGetObjectVersionClass;
				};
				
				//
				class GetObjectVersionClass : public std::enable_shared_from_this<GetObjectVersionClass> {
				public:
					//
					GetObjectVersionClass(const S3BucketImplPtr& bucket,
										  const std::string& objectKey,
										  const std::string& version,
										  const s3::GetS3ObjectResponseBlockPtr& responseBlock,
										  const s3::S3CompletionBlockPtr& completion) :
					mBucket(bucket),
					mObjectKey(objectKey),
					mVersion(version),
					mResponseBlock(responseBlock),
					mCompletion(completion),
					mLatestResult(s3::S3Result::kUnknown),
					mRetries(0),
					mAccessDeniedRetries(0),
					mSleepInterval(1),
					mSleepIntervalStep(2) {
					}
					
					//
					void GetObjectVersionWithRetry(const HermitPtr& h_) {
						if (CHECK_FOR_ABORT(h_)) {
							mCompletion->Call(h_, s3::S3Result::kCanceled);
							return;
						}
						
						if (mRetries > 0) {
							s3::S3NotificationParams params("GetObjectVersion", mRetries, mLatestResult);
							NOTIFY(h_, s3::kS3RetryNotification, &params);
						}
						
						mBucket->RefreshSigningKeyIfNeeded();
						
						auto completion = std::make_shared<GetObjectVersionCompletion>(shared_from_this());
						s3::GetS3Object(h_,
										mBucket->mAWSPublicKey,
										mBucket->mAWSSigningKey.data(),
										mBucket->mAWSSigningKey.size(),
										mBucket->mAWSRegion,
										mBucket->mBucketName,
										mObjectKey,
										mResponseBlock,
										completion);
					}
					
					//
					void Completion(const HermitPtr& h_, const s3::S3Result& result) {
						mLatestResult = result;
						
						if (!ShouldRetry(result)) {
							if (mRetries > 0) {
								s3::S3NotificationParams params("GetObjectVersion", mRetries, result);
								NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
							}
							ProcessResult(h_, result);
							return;
						}
						if (++mRetries == S3BucketImpl::kMaxRetries) {
							s3::S3NotificationParams params("GetObjectVersion", mRetries, result);
							NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
							
							NOTIFY_ERROR(h_, "GetObjectVersionFromS3Bucket: maximum retries exceeded, most recent result:", (int)result);
							mCompletion->Call(h_, result);
							return;
						}
						
						int fifthSecondIntervals = mSleepInterval * 5;
						for (int i = 0; i < fifthSecondIntervals; ++i) {
							if (CHECK_FOR_ABORT(h_)) {
								mCompletion->Call(h_, s3::S3Result::kCanceled);
								return;
							}
							std::this_thread::sleep_for(std::chrono::milliseconds(200));
						}
						mSleepInterval += mSleepIntervalStep;
						mSleepIntervalStep += 2;
						
						GetObjectVersionWithRetry(h_);
					}
					
					//
					bool ShouldRetry(const s3::S3Result& result) {
						if ((result == s3::S3Result::kTimedOut) ||
							(result == s3::S3Result::kNetworkConnectionLost) ||
							(result == s3::S3Result::kChecksumMismatch) ||
							(result == s3::S3Result::k500InternalServerError) ||
							(result == s3::S3Result::k503ServiceUnavailable) ||
							(result == s3::S3Result::kS3InternalError) ||
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
					void ProcessResult(const HermitPtr& h_, const s3::S3Result& result) {
						if (result == s3::S3Result::kCanceled) {
							mCompletion->Call(h_, result);
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mCompletion->Call(h_, result);
							return;
						}
						if (result == s3::S3Result::k404EntityNotFound) {
							mCompletion->Call(h_, result);
							return;
						}
						NOTIFY_ERROR(h_, "GetObjectVersionFromS3Bucket: error result encountered:", (int)result);
						if (result == s3::S3Result::kAuthorizationHeaderMalformed) {
							// this *might* actually be caused by the bucket being deleted. sometimes S3 gives us
							// 404 and sometimes it gives us this unhelpful response about the region being wrong in
							// the auth info.
							s3::GetS3BucketLocationCallbackClass location;
							GetS3BucketLocation(h_,
												mBucket->mBucketName,
												mBucket->mAWSPublicKey,
												mBucket->mAWSPrivateKey,
												location);
							
							if (location.mResult == s3::S3Result::kCanceled) {
								mCompletion->Call(h_, s3::S3Result::kCanceled);
								return;
							}
							if (location.mResult == s3::S3Result::k404NoSuchBucket) {
								NOTIFY_ERROR(h_, "GetObjectVersionFromS3Bucket: treating AuthorizationHeaderMalformed as NoSuchBucket");
								mCompletion->Call(h_, s3::S3Result::k404NoSuchBucket);
								return;
							}
							// we only care about the above responses from GetS3BucketLocation, otherwise
							// we fall through to reporting the original error on the get object call.
						}
						mCompletion->Call(h_, result);
					}
					
					//
					S3BucketImplPtr mBucket;
					std::string mObjectKey;
					std::string mVersion;
					s3::GetS3ObjectResponseBlockPtr mResponseBlock;
					s3::S3CompletionBlockPtr mCompletion;
					s3::S3Result mLatestResult;
					int mRetries;
					int mAccessDeniedRetries;
					int mSleepInterval;
					int mSleepIntervalStep;
				};
				
				//
				void GetObjectVersionCompletion::Call(const HermitPtr& h_, const s3::S3Result& result) {
					mGetObjectVersionClass->Completion(h_, result);
				}
				
			} // namespace
			
			//
			void S3BucketImpl::GetObjectVersion(const HermitPtr& h_,
												const std::string& inS3ObjectKey,
												const std::string& inVersion,
												const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
												const s3::S3CompletionBlockPtr& inCompletion) {
				auto getObjectVersion = std::make_shared<GetObjectVersionClass>(shared_from_this(),
																				inS3ObjectKey,
																				inVersion,
																				inResponseBlock,
																				inCompletion);
				getObjectVersion->GetObjectVersionWithRetry(h_);
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
