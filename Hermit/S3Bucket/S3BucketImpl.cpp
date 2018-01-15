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
#include "Hermit/S3/GenerateAWS4SigningKey.h"
#include "Hermit/S3/GetS3BucketLocation.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
    namespace s3bucket {
        namespace impl {
            namespace S3BucketImpl_Impl {
                
                //
                typedef std::shared_ptr<S3BucketImpl> S3BucketImplPtr;

                //
                WithS3BucketStatus S3ResultToWithS3BucketStatus(s3::S3Result result) {
                    if (result == s3::S3Result::kSuccess) {
                        return WithS3BucketStatus::kSuccess;
                    }
                    if (result == s3::S3Result::kCanceled) {
                        return WithS3BucketStatus::kCancel;
                    }
                    if (result == s3::S3Result::kInvalidAccessKey) {
                        return WithS3BucketStatus::kInvalidAccessKey;
                    }
                    if (result == s3::S3Result::kSignatureDoesNotMatch) {
                        return WithS3BucketStatus::kSignatureDoesNotMatch;
                    }
                    if (result == s3::S3Result::k403AccessDenied) {
                        return WithS3BucketStatus::kAccessDenied;
                    }
                    if (result == s3::S3Result::k404NoSuchBucket) {
                        return WithS3BucketStatus::kNoSuchBucket;
                    }
                    if (result == s3::S3Result::kNoNetworkConnection) {
                        return WithS3BucketStatus::kNoNetworkConnection;
                    }
                    if (result == s3::S3Result::kNetworkConnectionLost) {
                        return WithS3BucketStatus::kNetworkConnectionLost;
                    }
                    if (result == s3::S3Result::kTimedOut) {
                        return WithS3BucketStatus::kTimedOut;
                    }
                    return WithS3BucketStatus::kError;
                }
                
                //
                class GetBucketLocationClass;
                typedef std::shared_ptr<GetBucketLocationClass> GetBucketLocationClassPtr;
                
                //
                class GetBucketLocationCompletion : public s3::GetS3BucketLocationCompletion {
                public:
                    //
                    GetBucketLocationCompletion(const GetBucketLocationClassPtr& getBucketLocationClass) :
                    mGetBucketLocationClass(getBucketLocationClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_,
                                      const s3::S3Result& result,
                                      const std::string& location) override;
                    
                    //
                    GetBucketLocationClassPtr mGetBucketLocationClass;
                };
                
                //
                class GetBucketLocationClass : public std::enable_shared_from_this<GetBucketLocationClass> {
                public:
                    //
                    GetBucketLocationClass(const S3BucketImplPtr& bucket, const InitS3BucketCompletionPtr& completion) :
                    mBucket(bucket),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void GetBucketLocationWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, WithS3BucketStatus::kCancel);
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("GetBucketLocation", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                        
                        auto completion = std::make_shared<GetBucketLocationCompletion>(shared_from_this());
                        GetS3BucketLocation(h_,
                                            mBucket->mBucketName,
                                            mBucket->mAWSPublicKey,
                                            mBucket->mAWSPrivateKey,
                                            completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_, const s3::S3Result& result, const std::string& location) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("GetBucketLocation", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result, location);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("GetBucketLocation", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, S3ResultToWithS3BucketStatus(result));
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, WithS3BucketStatus::kCancel);
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        GetBucketLocationWithRetry(h_);
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
                    void ProcessResult(const HermitPtr& h_, const s3::S3Result& result, const std::string& location) {
                        if (result == s3::S3Result::kSuccess) {
                            mBucket->mAWSRegion = location;
                        
                            s3::GenerateAWS4SigningKeyCallbackClass keyCallback;
                            GenerateAWS4SigningKey(mBucket->mAWSPrivateKey, mBucket->mAWSRegion, keyCallback);
                            mBucket->mAWSSigningKey = keyCallback.mKey;
                            mBucket->mSigningKeyDateString = keyCallback.mDateString;
                        
                            mCompletion->Call(h_, WithS3BucketStatus::kSuccess);
                            return;
                        }
                        mCompletion->Call(h_, S3ResultToWithS3BucketStatus(result));
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    InitS3BucketCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void GetBucketLocationCompletion::Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& location) {
                    mGetBucketLocationClass->Completion(h_, result, location);
                }

            } // namespace S3BucketImpl_Impl
            using namespace S3BucketImpl_Impl;
            
            //
            void S3BucketImpl::Init(const HermitPtr& h_, const InitS3BucketCompletionPtr& completion) {
                auto getBucketLocation = std::make_shared<GetBucketLocationClass>(shared_from_this(), completion);
                getBucketLocation->GetBucketLocationWithRetry(h_);
            }
            
            //
            void S3BucketImpl::RefreshSigningKeyIfNeeded() {
                time_t now;
                time(&now);
                tm globalTime;
                gmtime_r(&now, &globalTime);
                char dateBuf[32];
                strftime(dateBuf, 32, "%Y%m%d", &globalTime);
                std::string date(dateBuf);
                
                ThreadLockScope lock(mLock);
                if (date != mSigningKeyDateString) {
                    s3::GenerateAWS4SigningKeyCallbackClass keyCallback;
                    GenerateAWS4SigningKey(mAWSPrivateKey, mAWSRegion, keyCallback);
                    mAWSSigningKey = keyCallback.mKey;
                    mSigningKeyDateString = keyCallback.mDateString;
                }
            }
            
        } // namespace impl
    } // namespace s3bucket
} // namespace hermit

