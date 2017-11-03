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
#include "PutMultipartObjectToS3Bucket.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
			namespace {
				
				//
				//
				typedef std::pair<int, std::string> PartInfoPair;
				typedef std::vector<PartInfoPair> PartInfoVector;
				
				//
				//
				class PartInfoFunction
				:
				public s3::CompleteS3MultipartUploadPartInfoFunction
				{
				public:
					//
					//
					PartInfoFunction(const PartInfoVector& inPartInfos) :
					mPartInfos(inPartInfos) {
					}
					
					//
					//
					bool Function(const s3::CompleteS3MultipartUploadPartInfoCallbackRef& inCallback) {
						auto end = mPartInfos.end();
						for (auto it = mPartInfos.begin(); it != end; ++it) {
							if (!inCallback.Call(it->first, it->second)) {
								return false;
							}
						}
						return true;
					}
					
					//
					//
					const PartInfoVector& mPartInfos;
				};
				
				// InitiateMultipartUpload operation class
				class InitiateMultipartUpload {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					std::string mDataSHA256Hex;
					s3::InitiateS3MultipartUploadCallbackRef mCallback;
					int mAccessDeniedRetries;
					std::string mUploadId;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "InitiateMultipartUpload";
					}
					
					//
					InitiateMultipartUpload(S3BucketImpl& bucket,
											const std::string& objectKey,
											const std::string& inDataSHA256Hex,
											const s3::InitiateS3MultipartUploadCallbackRef& callback) :
					mBucket(bucket),
					mObjectKey(objectKey),
					mDataSHA256Hex(inDataSHA256Hex),
					mCallback(callback),
					mAccessDeniedRetries(0) {
					}
					
					//
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						s3::InitiateS3MultipartUploadCallbackClass callback;
						InitiateS3MultipartUpload(h_,
												  mBucket.mAWSPublicKey,
												  mBucket.mAWSSigningKey.data(),
												  mBucket.mAWSSigningKey.size(),
												  mBucket.mAWSRegion,
												  mBucket.mBucketName,
												  mObjectKey,
												  mDataSHA256Hex,
												  callback);
						
						if (callback.mResult == s3::S3Result::kSuccess) {
							mUploadId = callback.mUploadID;
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
					
					//
					void ProcessResult(const HermitPtr& h_, const ResultType& result) {
						
						if (result == s3::S3Result::kCanceled) {
							mCallback.Call(result, "");
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mCallback.Call(result, mUploadId);
							return;
						}
						NOTIFY_ERROR(h_, "InitiateMultipartUpload: error result encountered:", (int)result);
						mCallback.Call(result, "");
					}
					
					//
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "InitiateMultipartUpload: maximum retries exceeded, most recent result:", (int)result);
						mCallback.Call(result, "");
					}
					
					//
					void Canceled() {
						mCallback.Call(s3::S3Result::kCanceled, "");
					}
				};
				
				// UploadMultipartPart operation class
				class UploadMultipartPart {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					std::string mUploadId;
					int32_t mPartNumber;
					DataBuffer mPartData;
					s3::UploadS3MultipartPartCallbackRef mCallback;
					int mAccessDeniedRetries;
					std::string mETag;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "UploadMultipartPart";
					}
					
					//
					UploadMultipartPart(S3BucketImpl& bucket,
										const std::string& objectKey,
										const std::string& uploadId,
										int32_t partNumber,
										const DataBuffer& partData,
										const s3::UploadS3MultipartPartCallbackRef& callback) :
					mBucket(bucket),
					mObjectKey(objectKey),
					mUploadId(uploadId),
					mPartNumber(partNumber),
					mPartData(partData),
					mCallback(callback),
					mAccessDeniedRetries(0) {
					}
					
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						s3::UploadS3MultipartPartCallbackClass callback;
						UploadS3MultipartPart(h_,
											  mBucket.mAWSPublicKey,
											  mBucket.mAWSSigningKey,
											  mBucket.mAWSRegion,
											  mBucket.mBucketName,
											  mObjectKey,
											  mUploadId,
											  mPartNumber,
											  mPartData,
											  callback);
						
						if (callback.mResult == s3::S3Result::kSuccess) {
							mETag = callback.mETag;
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
							mCallback.Call(result, mETag);
							return;
						}
						NOTIFY_ERROR(h_, "UploadMultipartPart: error result encountered:", (int)result);
						mCallback.Call(result, "");
					}
					
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "UploadMultipartPart: maximum retries exceeded, most recent result:", (int)result);
						mCallback.Call(result, "");
					}
					
					void Canceled() {
						mCallback.Call(s3::S3Result::kCanceled, "");
					}
				};
				
				// CompleteMultipartUpload operation class
				class CompleteMultipartUpload {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					std::string mUploadId;
					const PartInfoVector& mPartInfos;
					s3::S3Result mResult;
					int mAccessDeniedRetries;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "CompleteMultipartUpload";
					}
					
					//
					CompleteMultipartUpload(S3BucketImpl& bucket,
											const std::string& objectKey,
											const std::string& uploadId,
											const PartInfoVector& partInfos) :
					mBucket(bucket),
					mObjectKey(objectKey),
					mUploadId(uploadId),
					mPartInfos(partInfos),
					mResult(s3::S3Result::kUnknown),
					mAccessDeniedRetries(0) {
					}
					
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						PartInfoFunction partInfoFunction(mPartInfos);
						return CompleteS3MultipartUpload(h_,
														 mBucket.mAWSPublicKey,
														 mBucket.mAWSSigningKey,
														 mBucket.mAWSRegion,
														 mBucket.mBucketName,
														 mObjectKey,
														 mUploadId,
														 partInfoFunction);
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
							mResult = result;
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mResult = result;
							return;
						}
						NOTIFY_ERROR(h_, "CompleteMultipartUpload: error result encountered:", (int)result);
						mResult = result;
					}
					
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "CompleteMultipartUpload: maximum retries exceeded, most recent result:", (int)result);
						mResult = result;
					}
					
					void Canceled() {
						mResult = s3::S3Result::kCanceled;
					}
					
					s3::S3Result GetResult() const {
						return mResult;
					}
				};
				
				// AbortMultipartUpload operation class
				class AbortMultipartUpload {
				private:
					S3BucketImpl& mBucket;
					std::string mObjectKey;
					std::string mUploadId;
					s3::S3Result mResult;
					int mAccessDeniedRetries;
					
				public:
					typedef s3::S3Result ResultType;
					static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
					static const int kMaxRetries = S3BucketImpl::kMaxRetries;
					
					//
					const char* OpName() const {
						return "AbortMultipartUpload";
					}
					
					//
					AbortMultipartUpload(S3BucketImpl& bucket,
										 const std::string& objectKey,
										 const std::string& uploadId) :
					mBucket(bucket),
					mObjectKey(objectKey),
					mUploadId(uploadId),
					mResult(s3::S3Result::kUnknown),
					mAccessDeniedRetries(0) {
					}
					
					ResultType AttemptOnce(const HermitPtr& h_) {
						mBucket.RefreshSigningKeyIfNeeded();
						
						return s3::AbortS3MultipartUpload(h_,
														  mBucket.mAWSPublicKey,
														  mBucket.mAWSSigningKey,
														  mBucket.mAWSRegion,
														  mBucket.mBucketName,
														  mObjectKey,
														  mUploadId);
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
							mResult = result;
							return;
						}
						if (result == s3::S3Result::kSuccess) {
							mResult = result;
							return;
						}
						NOTIFY_ERROR(h_, "AbortMultipartUpload: error result encountered:", (int)result);
						mResult = result;
					}
					
					void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
						NOTIFY_ERROR(h_, "AbortMultipartUpload: maximum retries exceeded, most recent result:", (int)result);
						mResult = result;
					}
					
					void Canceled() {
						mResult = s3::S3Result::kCanceled;
					}
					
					s3::S3Result GetResult() const {
						return mResult;
					}
				};
				
				// proxy to avoid cancel during abort
				class AbortNotificationProxy : public Hermit
				{
				public:
					//
					AbortNotificationProxy(const HermitPtr& h_)
					:
					mH_(h_) {
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
				
			} // private namespace
			
			//
			//
			void PutMultipartObjectToS3Bucket(const HermitPtr& h_,
											  S3BucketImpl& inS3Bucket,
											  const std::string& inS3ObjectKey,
											  const DataBuffer& inData,
											  const bool& inUseReducedRedundancyStorage,
											  const s3::PutS3ObjectCallbackRef& inCallback) {
				
				//	This is handled internally for the PutS3Object case, but we need to do it here
				//	for the multipart upload case.
				std::string dataSHA256;
				encoding::CalculateSHA256(std::string(inData.first, inData.second), dataSHA256);
				if (dataSHA256.empty()) {
					NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: CalculateSHA256 failed.");
					inCallback.Call(s3::S3Result::kError, "");
					return;
				}
				
				std::string dataSHA256Hex;
				string::BinaryStringToHex(dataSHA256, dataSHA256Hex);
				if (dataSHA256Hex.empty()) {
					NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: BinaryStringToHex failed.");
					inCallback.Call(s3::S3Result::kError, "");
					return;
				}
				
				s3::InitiateS3MultipartUploadCallbackClass initiateCallback;
				InitiateMultipartUpload initiate(inS3Bucket, inS3ObjectKey, dataSHA256Hex, initiateCallback);
				s3::RetryClassT<InitiateMultipartUpload> retry;
				retry.AttemptWithRetry(h_, initiate);
				if (initiateCallback.mResult == s3::S3Result::kCanceled) {
					inCallback.Call(s3::S3Result::kCanceled, "");
					return;
				}
				if (initiateCallback.mResult != s3::S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: InitiateMultipartUpload failed.");
					inCallback.Call(initiateCallback.mResult, "");
					return;
				}
				std::string uploadId(initiateCallback.mUploadID);
				
				bool success = true;
				uint64_t partSize = 6 * 1024 * 1024;
				int32_t partCount = (int32_t)(inData.second / partSize);
				PartInfoVector partInfos;
				for (int32_t partNumber = 1; partNumber <= partCount; ++partNumber) {
					uint64_t thisPartSize = partSize;
					if (partNumber == partCount) {
						thisPartSize += inData.second % partSize;
					}
					
					s3::UploadS3MultipartPartCallbackClass uploadCallback;
					UploadMultipartPart upload(inS3Bucket,
											   inS3ObjectKey,
											   uploadId,
											   partNumber,
											   DataBuffer(inData.first + (partNumber - 1) * partSize, thisPartSize),
											   uploadCallback);
					s3::RetryClassT<UploadMultipartPart> retry;
					retry.AttemptWithRetry(h_, upload);
					if (uploadCallback.mResult == s3::S3Result::kCanceled) {
						inCallback.Call(s3::S3Result::kCanceled, "");
						success = false;
						break;
					}
					if (uploadCallback.mResult != s3::S3Result::kSuccess) {
						NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: UploadMultipartPart failed.");
						inCallback.Call(uploadCallback.mResult, "");
						success = false;
						break;
					}
					partInfos.push_back(PartInfoPair(partNumber, uploadCallback.mETag));
				}
				
				if (success) {
					CompleteMultipartUpload complete(inS3Bucket,
													 inS3ObjectKey,
													 uploadId,
													 partInfos);
					s3::RetryClassT<CompleteMultipartUpload> retry;
					retry.AttemptWithRetry(h_, complete);
					auto result = complete.GetResult();
					if (result == s3::S3Result::kCanceled) {
						inCallback.Call(s3::S3Result::kCanceled, "");
						success = false;
					}
					else if (result != s3::S3Result::kSuccess) {
						NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: CompleteMultipartUpload failed.");
						inCallback.Call(result, "");
						success = false;
					}
				}
				
				if (!success) {
					AbortMultipartUpload abort(inS3Bucket, inS3ObjectKey, uploadId);
					s3::RetryClassT<AbortMultipartUpload> retry;
					
					// we specifically want to avoid a cancel here because there can be an S3 cost
					// associated for partially uploaded multipart objects unless abort is called,
					// so we pass a special proxy here
					auto proxy = std::make_shared<AbortNotificationProxy>(h_);
					retry.AttemptWithRetry(proxy, abort);
					if (abort.GetResult() != s3::S3Result::kSuccess) {
						NOTIFY_ERROR(h_, "PutMultipartObjectToS3Bucket: AbortMultipartUpload failed.");
						// the main callback has already been called with the primary result already,
						// it would be a mistake to call it again
					}
				}
				else {
					inCallback.Call(s3::S3Result::kSuccess, ""); // version?
				}
			}
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit
