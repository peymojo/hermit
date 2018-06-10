//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Hermit/Foundation/Notification.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
    namespace s3bucket {
        namespace impl {
            namespace S3BucketImpl_IsVersioningEnabled_Impl {
                
                //
                typedef std::shared_ptr<S3BucketImpl> S3BucketImplPtr;
                
                //
                class IsVersioningEnabledClass;
                typedef std::shared_ptr<IsVersioningEnabledClass> IsVersioningEnabledClassPtr;
                
                //
                class IsVersioningEnabledCompletion : public s3::S3GetBucketVersioningCompletion {
                public:
                    //
                    IsVersioningEnabledCompletion(const IsVersioningEnabledClassPtr& isVersioningEnabledClass) :
                    mIsVersioningEnabledClass(isVersioningEnabledClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_,
                                      const s3::S3Result& result,
                                      const s3::S3BucketVersioningStatus& status) override;
                    
                    //
                    IsVersioningEnabledClassPtr mIsVersioningEnabledClass;
                };
                
                //
                class IsVersioningEnabledClass : public std::enable_shared_from_this<IsVersioningEnabledClass> {
                public:
                    //
                    IsVersioningEnabledClass(const S3BucketImplPtr& bucket,
                                             const s3::S3GetBucketVersioningCompletionPtr& completion) :
                    mBucket(bucket),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void IsVersioningEnabledWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, s3::S3Result::kCanceled, s3::S3BucketVersioningStatus::kUnknown);
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("IsVersioningEnabled", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                                                
                        auto completion = std::make_shared<IsVersioningEnabledCompletion>(shared_from_this());
                        s3::S3GetBucketVersioning(h_,
												  mBucket->mHTTPSession,
                                                  mBucket->mBucketName,
                                                  mBucket->mAWSPublicKey,
                                                  mBucket->mAWSPrivateKey,
                                                  completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_,
                                    const s3::S3Result& result,
                                    const s3::S3BucketVersioningStatus& status) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("IsVersioningEnabled", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result, status);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("IsVersioningEnabled", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, result, s3::S3BucketVersioningStatus::kUnknown);
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, s3::S3Result::kCanceled, s3::S3BucketVersioningStatus::kUnknown);
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        IsVersioningEnabledWithRetry(h_);
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
                    void ProcessResult(const HermitPtr& h_,
                                       const s3::S3Result& result,
                                       const s3::S3BucketVersioningStatus& status) {
                        mCompletion->Call(h_, result, status);
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    s3::S3GetBucketVersioningCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void IsVersioningEnabledCompletion::Call(const HermitPtr& h_,
                                                         const s3::S3Result& result,
                                                         const s3::S3BucketVersioningStatus& status) {
                    mIsVersioningEnabledClass->Completion(h_, result, status);
                }
                
            } // namespace S3BucketImpl_IsVersioningEnabled_Impl
            using namespace S3BucketImpl_IsVersioningEnabled_Impl;
            
            //
            void S3BucketImpl::IsVersioningEnabled(const HermitPtr& h_,
                                                   const s3::S3GetBucketVersioningCompletionPtr& completion) {
                auto isVersioningEnabled = std::make_shared<IsVersioningEnabledClass>(shared_from_this(), completion);
                isVersioningEnabled->IsVersioningEnabledWithRetry(h_);
            }
            
        } // namespace impl
    } // namespace s3bucket
} // namespace hermit

