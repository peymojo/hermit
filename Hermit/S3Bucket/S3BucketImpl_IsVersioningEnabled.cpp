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
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
namespace s3bucket {
namespace impl {

namespace {

	//
	class VersioningCallback
		:
		public s3::S3GetBucketVersioningCallback
	{
	public:
		//
		//
		VersioningCallback(
			const s3::S3GetBucketVersioningCallbackRef& inCallback)
			:
			mCallback(inCallback),
			mResult(s3::S3Result::kUnknown) {
		}
		
		//
		//
		bool Function(const s3::S3Result& inResult, const s3::S3BucketVersioningStatus& inStatus) {
			mResult = inResult;
			if (inResult == s3::S3Result::kSuccess) {
				return mCallback.Call(inResult, inStatus);
			}
			return true;
		}
		
		//
		//
		const s3::S3GetBucketVersioningCallbackRef& mCallback;
		s3::S3Result mResult;
	};

	// IsVersioningEnabled operation class
	class IsVersioningEnabledOp {
	private:
		S3BucketImpl& mBucket;
		s3::S3GetBucketVersioningCallbackRef mCallback;
		int mAccessDeniedRetries;

	public:
		typedef s3::S3Result ResultType;
		static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
		static const int kMaxRetries = S3BucketImpl::kMaxRetries;

		//
		const char* OpName() const {
			return "IsVersioningEnabled";
		}
		
		//
		IsVersioningEnabledOp(
			S3BucketImpl& bucket,
			const s3::S3GetBucketVersioningCallbackRef& callback)
			:
			mBucket(bucket),
			mCallback(callback),
			mAccessDeniedRetries(0) {
		}
		
		//
		ResultType AttemptOnce(const HermitPtr& h_) {
			mBucket.RefreshSigningKeyIfNeeded();

			VersioningCallback callback(mCallback);
			S3GetBucketVersioning(h_,
								  mBucket.mBucketName,
								  mBucket.mAWSPublicKey,
								  mBucket.mAWSPrivateKey,
								  callback);
			
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
				mCallback.Call(result, s3::S3BucketVersioningStatus::kUnknown);
				return;
			}
			if (result == s3::S3Result::kSuccess) {
				// already called in the passthrough case
				return;
			}
			NOTIFY_ERROR(h_, "IsVersioningEnabled: error result encountered:", (int)result);
			mCallback.Call(result, s3::S3BucketVersioningStatus::kUnknown);
		}
		
		//
		void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
			NOTIFY_ERROR(h_, "IsVersioningEnabled: maximum retries exceeded, most recent result:", (int)result);
			mCallback.Call(result, s3::S3BucketVersioningStatus::kUnknown);
		}

		//
		void Canceled() {
			mCallback.Call(s3::S3Result::kCanceled, s3::S3BucketVersioningStatus::kUnknown);
		}
	};

} // private namespace
	
//
void S3BucketImpl::IsVersioningEnabled(const HermitPtr& h_,
									   const s3::S3GetBucketVersioningCallbackRef& inCallback) {
	IsVersioningEnabledOp isEnabled(*this, inCallback);
	s3::RetryClassT<IsVersioningEnabledOp> retry;
	retry.AttemptWithRetry(h_, isEnabled);
}

} // namespace impl
} // namespace s3bucket
} // namespace hermit
