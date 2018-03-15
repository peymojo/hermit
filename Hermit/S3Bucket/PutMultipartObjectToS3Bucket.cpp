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

#include <vector>
#include "Hermit/Encoding/CalculateSHA256.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/BinaryStringToHex.h"
#include "Hermit/S3/AbortS3MultipartUpload.h"
#include "Hermit/S3/CompleteS3MultipartUpload.h"
#include "Hermit/S3/InitiateS3MultipartUpload.h"
#include "Hermit/S3/S3RetryClass.h"
#include "Hermit/S3/UploadS3MultipartPart.h"
#include "S3BucketImpl.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			namespace S3BucketImpl_PutMultipartObjectToS3Bucket_Impl {
								
                //
                typedef std::shared_ptr<S3BucketImpl> S3BucketImplPtr;
 
                // proxy to avoid cancel during abort
                class AbortNotificationProxy : public Hermit {
                public:
                    //
                    AbortNotificationProxy(const HermitPtr& h_) : mH_(h_) {
                    }
                    
                    //
                    virtual bool ShouldAbort() override {
                        return false;
                    }
                    
                    //
                    virtual void Notify(const char* name, const void* param) override {
                        NOTIFY(mH_, name, param);
                    }
                    
                    //
                    HermitPtr mH_;
                };
                
                //
                class AbortUploadClass;
                typedef std::shared_ptr<AbortUploadClass> AbortUploadClassPtr;
                
                //
                class AbortUploadCompletion : public s3::S3CompletionBlock {
                public:
                    //
                    AbortUploadCompletion(const AbortUploadClassPtr& abortUploadClass) :
                    mAbortUploadClass(abortUploadClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override;
                    
                    //
                    AbortUploadClassPtr mAbortUploadClass;
                };
                
                //
                class AbortUploadClass : public std::enable_shared_from_this<AbortUploadClass> {
                public:
                    //
                    AbortUploadClass(const S3BucketImplPtr& bucket,
                                     const std::string& objectKey,
                                     const std::string& uploadId,
                                     const s3::S3Result& originalResult,
                                     const s3::PutS3ObjectCompletionPtr& completion) :
                    mBucket(bucket),
                    mObjectKey(objectKey),
                    mUploadId(uploadId),
                    mOriginalResult(originalResult),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void AbortUploadWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("AbortUpload", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                        
                        mBucket->RefreshSigningKeyIfNeeded();
                        
                        auto completion = std::make_shared<AbortUploadCompletion>(shared_from_this());
                        s3::AbortS3MultipartUpload(h_,
                                                   mBucket->mAWSPublicKey,
                                                   mBucket->mAWSSigningKey,
                                                   mBucket->mAWSRegion,
                                                   mBucket->mBucketName,
                                                   mObjectKey,
                                                   mUploadId,
                                                   completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_, const s3::S3Result& result) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("AbortUpload", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("AbortUpload", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, result, "");
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        AbortUploadWithRetry(h_);
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
                        if (result != s3::S3Result::kSuccess) {
                            // Log this result but this isn't the result we pass up the chain,
                            // we pass the original issue up.
                            NOTIFY_ERROR(h_, "AbortMultipartUpload failed");
                        }
                        mCompletion->Call(h_, mOriginalResult, "");
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    std::string mObjectKey;
                    std::string mUploadId;
                    s3::S3Result mOriginalResult;
                    s3::PutS3ObjectCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void AbortUploadCompletion::Call(const HermitPtr& h_, const s3::S3Result& result) {
                    mAbortUploadClass->Completion(h_, result);
                }

                
                
                

                //
                class CompleteUploadClass;
                typedef std::shared_ptr<CompleteUploadClass> CompleteUploadClassPtr;
                
                //
                class CompleteUploadCompletion : public s3::S3CompletionBlock {
                public:
                    //
                    CompleteUploadCompletion(const CompleteUploadClassPtr& completeUploadClass) :
                    mCompleteUploadClass(completeUploadClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override;
                    
                    //
                    CompleteUploadClassPtr mCompleteUploadClass;
                };
                
                //
                class CompleteUploadClass : public std::enable_shared_from_this<CompleteUploadClass> {
                public:
                    //
                    CompleteUploadClass(const S3BucketImplPtr& bucket,
                                        const std::string& objectKey,
                                        const std::string& uploadId,
                                        const s3::PartVector& parts,
                                        const s3::PutS3ObjectCompletionPtr& completion) :
                    mBucket(bucket),
                    mObjectKey(objectKey),
                    mUploadId(uploadId),
                    mParts(parts),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void CompleteUploadWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("CompleteUpload", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                        
                        mBucket->RefreshSigningKeyIfNeeded();
                        
                        auto completion = std::make_shared<CompleteUploadCompletion>(shared_from_this());
                        CompleteS3MultipartUpload(h_,
                                                  mBucket->mAWSPublicKey,
                                                  mBucket->mAWSSigningKey,
                                                  mBucket->mAWSRegion,
                                                  mBucket->mBucketName,
                                                  mObjectKey,
                                                  mUploadId,
                                                  mParts,
                                                  completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_, const s3::S3Result& result) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("CompleteUpload", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("CompleteUpload", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, result, "");
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        CompleteUploadWithRetry(h_);
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
                        if (result == s3::S3Result::kSuccess) {
                            mCompletion->Call(h_, result, "");
                            return;
                        }
                        if (result != s3::S3Result::kCanceled) {
                            NOTIFY_ERROR(h_, "CompleteMultipartUpload failed.");
                        }
                        // we specifically want to avoid a cancel here because there can be an S3 cost
                        // associated for partially uploaded multipart objects unless abort is called,
                        // so we pass a special proxy here
                        auto proxy = std::make_shared<AbortNotificationProxy>(h_);
                        auto aborter = std::make_shared<AbortUploadClass>(mBucket, mObjectKey, mUploadId, result, mCompletion);
                        aborter->AbortUploadWithRetry(proxy);
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    std::string mObjectKey;
                    std::string mUploadId;
                    s3::PartVector mParts;
                    s3::PutS3ObjectCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void CompleteUploadCompletion::Call(const HermitPtr& h_, const s3::S3Result& result) {
                    mCompleteUploadClass->Completion(h_, result);
                }
                

                
                
                
                
                //
                class UploadPartClass;
                typedef std::shared_ptr<UploadPartClass> UploadPartClassPtr;
                
                //
                class UploadPartCompletion : public s3::UploadS3MultipartPartCompletion {
                public:
                    //
                    UploadPartCompletion(const UploadPartClassPtr& uploadPartClass) :
                    mUploadPartClass(uploadPartClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& eTag) override;
                    
                    //
                    UploadPartClassPtr mUploadPartClass;
                };
                
                //
                class UploadPartClass : public std::enable_shared_from_this<UploadPartClass> {
                public:
                    //
                    UploadPartClass(const S3BucketImplPtr& bucket,
                                    const std::string& objectKey,
                                    const SharedBufferPtr& data,
                                    const std::string& uploadId,
                                    int64_t calculatedPartSize,
                                    int32_t numberOfParts,
                                    int32_t thisPartNumber,
                                    const s3::PartVector& parts,
                                    const s3::PutS3ObjectCompletionPtr& completion) :
                    mBucket(bucket),
                    mObjectKey(objectKey),
                    mData(data),
                    mUploadId(uploadId),
                    mCalculatedPartSize(calculatedPartSize),
                    mNumberOfParts(numberOfParts),
                    mThisPartNumber(thisPartNumber),
                    mParts(parts),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void UploadPartWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("UploadPart", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                        
                        mBucket->RefreshSigningKeyIfNeeded();
                        
                        uint64_t thisPartSize = mCalculatedPartSize;
                        if (mThisPartNumber == mNumberOfParts) {
                            thisPartSize += mData->Size() % mCalculatedPartSize;
                        }
                        auto completion = std::make_shared<UploadPartCompletion>(shared_from_this());
                        UploadS3MultipartPart(h_,
                                              mBucket->mAWSPublicKey,
                                              mBucket->mAWSSigningKey,
                                              mBucket->mAWSRegion,
                                              mBucket->mBucketName,
                                              mObjectKey,
                                              mUploadId,
                                              mThisPartNumber,
                                              std::make_shared<SharedBuffer>(mData->Data() + (mThisPartNumber - 1) * mCalculatedPartSize, thisPartSize),
                                              completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_, const s3::S3Result& result, const std::string& eTag) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("UploadPart", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result, eTag);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("UploadPart", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, result, "");
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        UploadPartWithRetry(h_);
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
                    void ProcessResult(const HermitPtr& h_, const s3::S3Result& result, const std::string& eTag) {
                        if (result == s3::S3Result::kSuccess) {
                            mParts.push_back(std::make_pair(mThisPartNumber, eTag));
                            if (mThisPartNumber < mNumberOfParts) {
                                auto partUploader = std::make_shared<UploadPartClass>(mBucket,
                                                                                      mObjectKey,
                                                                                      mData,
                                                                                      mUploadId,
                                                                                      mCalculatedPartSize,
                                                                                      mNumberOfParts,
                                                                                      mThisPartNumber + 1,
                                                                                      mParts,
                                                                                      mCompletion);
                                partUploader->UploadPartWithRetry(h_);
                                return;
                            }
                            
                            auto completer = std::make_shared<CompleteUploadClass>(mBucket,
                                                                                   mObjectKey,
                                                                                   mUploadId,
                                                                                   mParts,
                                                                                   mCompletion);
                            completer->CompleteUploadWithRetry(h_);
                            return;
                        }
                        
                        if (result != s3::S3Result::kCanceled) {
                            NOTIFY_ERROR(h_, "UploadMultipartPart failed.");
                        }
                        
                        // we specifically want to avoid a cancel here because there can be an S3 cost
                        // associated for partially uploaded multipart objects unless abort is called,
                        // so we pass a special proxy here
                        auto proxy = std::make_shared<AbortNotificationProxy>(h_);
                        auto aborter = std::make_shared<AbortUploadClass>(mBucket, mObjectKey, mUploadId, result, mCompletion);
                        aborter->AbortUploadWithRetry(proxy);
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    std::string mObjectKey;
                    SharedBufferPtr mData;
                    std::string mUploadId;
                    int64_t mCalculatedPartSize;
                    int32_t mNumberOfParts;
                    int32_t mThisPartNumber;
                    s3::PartVector mParts;
                    s3::PutS3ObjectCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void UploadPartCompletion::Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& uploadId) {
                    mUploadPartClass->Completion(h_, result, uploadId);
                }

                
                
                
                
                
                //
                class InitiateUploadClass;
                typedef std::shared_ptr<InitiateUploadClass> InitiateUploadClassPtr;
                
                //
                class InitiateUploadCompletion : public s3::InitiateS3MultipartUploadCompletion {
                public:
                    //
                    InitiateUploadCompletion(const InitiateUploadClassPtr& initiateUploadClass) :
                    mInitiateUploadClass(initiateUploadClass) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& uploadId) override;
                    
                    //
                    InitiateUploadClassPtr mInitiateUploadClass;
                };
                
                //
                class InitiateUploadClass : public std::enable_shared_from_this<InitiateUploadClass> {
                public:
                    //
                    InitiateUploadClass(const S3BucketImplPtr& bucket,
                                        const std::string& objectKey,
                                        const SharedBufferPtr& data,
                                        const std::string& dataSHA256Hex,
                                        const s3::PutS3ObjectCompletionPtr& completion) :
                    mBucket(bucket),
                    mObjectKey(objectKey),
                    mData(data),
                    mDataSHA256Hex(dataSHA256Hex),
                    mCompletion(completion),
                    mLatestResult(s3::S3Result::kUnknown),
                    mRetries(0),
                    mAccessDeniedRetries(0),
                    mSleepInterval(1),
                    mSleepIntervalStep(2) {
                    }
                    
                    //
                    void InitiateUploadWithRetry(const HermitPtr& h_) {
                        if (CHECK_FOR_ABORT(h_)) {
                            mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                            return;
                        }
                        
                        if (mRetries > 0) {
                            s3::S3NotificationParams params("InitiateUpload", mRetries, mLatestResult);
                            NOTIFY(h_, s3::kS3RetryNotification, &params);
                        }
                        
                        mBucket->RefreshSigningKeyIfNeeded();
                        
                        auto completion = std::make_shared<InitiateUploadCompletion>(shared_from_this());
                        InitiateS3MultipartUpload(h_,
                                                  mBucket->mAWSPublicKey,
                                                  mBucket->mAWSSigningKey,
                                                  mBucket->mAWSRegion,
                                                  mBucket->mBucketName,
                                                  mObjectKey,
                                                  mDataSHA256Hex,
                                                  completion);
                    }
                    
                    //
                    void Completion(const HermitPtr& h_, const s3::S3Result& result, const std::string& uploadId) {
                        mLatestResult = result;
                        
                        if (!ShouldRetry(result)) {
                            if (mRetries > 0) {
                                s3::S3NotificationParams params("InitiateUpload", mRetries, result);
                                NOTIFY(h_, s3::kS3RetryCompleteNotification, &params);
                            }
                            ProcessResult(h_, result, uploadId);
                            return;
                        }
                        if (++mRetries == S3BucketImpl::kMaxRetries) {
                            s3::S3NotificationParams params("InitiateUpload", mRetries, result);
                            NOTIFY(h_, s3::kS3MaxRetriesExceededNotification, &params);
                            
                            NOTIFY_ERROR(h_, "Maximum retries exceeded, most recent result:", (int)result);
                            mCompletion->Call(h_, result, "");
                            return;
                        }
                        
                        int fifthSecondIntervals = mSleepInterval * 5;
                        for (int i = 0; i < fifthSecondIntervals; ++i) {
                            if (CHECK_FOR_ABORT(h_)) {
                                mCompletion->Call(h_, s3::S3Result::kCanceled, "");
                                return;
                            }
                            std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        }
                        mSleepInterval += mSleepIntervalStep;
                        mSleepIntervalStep += 2;
                        
                        InitiateUploadWithRetry(h_);
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
                    void ProcessResult(const HermitPtr& h_, const s3::S3Result& result, const std::string& uploadId) {
                        uint64_t calculatedPartSize = 6 * 1024 * 1024;
                        int32_t numberOfParts = (int32_t)(mData->Size() / calculatedPartSize);
                        auto partUploader = std::make_shared<UploadPartClass>(mBucket,
                                                                              mObjectKey,
                                                                              mData,
                                                                              uploadId,
                                                                              calculatedPartSize,
                                                                              numberOfParts,
                                                                              1,
                                                                              s3::PartVector(),
                                                                              mCompletion);
                        partUploader->UploadPartWithRetry(h_);
                    }
                    
                    //
                    S3BucketImplPtr mBucket;
                    std::string mObjectKey;
                    SharedBufferPtr mData;
                    std::string mDataSHA256Hex;
                    s3::PutS3ObjectCompletionPtr mCompletion;
                    s3::S3Result mLatestResult;
                    int mRetries;
                    int mAccessDeniedRetries;
                    int mSleepInterval;
                    int mSleepIntervalStep;
                };
                
                //
                void InitiateUploadCompletion::Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& uploadId) {
                    mInitiateUploadClass->Completion(h_, result, uploadId);
                }
                
			} // namespace S3BucketImpl_PutMultipartObjectToS3Bucket_Impl
            using namespace S3BucketImpl_PutMultipartObjectToS3Bucket_Impl;
			
			//
            void S3BucketImpl::PutMultipartObjectToS3Bucket(const HermitPtr& h_,
                                                            const std::string& s3ObjectKey,
                                                            const SharedBufferPtr& data,
                                                            const bool& useReducedRedundancyStorage, // TODO: currently ignored
                                                            const s3::PutS3ObjectCompletionPtr& completion) {
				
				//	This is handled internally for the PutS3Object case, but we need to do it here
				//	for the multipart upload case.
				std::string dataSHA256;
				encoding::CalculateSHA256(std::string(data->Data(), data->Size()), dataSHA256);
				if (dataSHA256.empty()) {
					NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: CalculateSHA256 failed.");
                    completion->Call(h_, s3::S3Result::kError, "");
					return;
				}
				
				std::string dataSHA256Hex;
				string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
				if (dataSHA256Hex.empty()) {
					NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: BinaryStringToHex failed.");
                    completion->Call(h_, s3::S3Result::kError, "");
					return;
				}
                
                auto initiateClass = std::make_shared<InitiateUploadClass>(shared_from_this(),
                                                                           s3ObjectKey,
                                                                           data,
                                                                           dataSHA256Hex,
                                                                           completion);
                initiateClass->InitiateUploadWithRetry(h_);
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
