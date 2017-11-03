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
#include "Hermit/S3/GenerateAWS4SigningKey.h"
#include "Hermit/S3/GetS3BucketLocation.h"
#include "Hermit/S3/S3RetryClass.h"
#include "S3BucketImpl.h"

namespace hermit {
namespace s3bucket {
namespace impl {

namespace {

	// GetBucketLocation operation class
	class GetBucketLocation {
	private:
		std::string mBucketName;
		std::string mAWSPublicKey;
		std::string mAWSPrivateKey;
		s3::GetS3BucketLocationCallbackRef mCallback;
		int mAccessDeniedRetries;
		std::string mLocation;

	public:
		typedef s3::S3Result ResultType;
		static const s3::S3Result kDefaultResult = s3::S3Result::kUnknown;
		static const int kMaxRetries = S3BucketImpl::kMaxRetries;

		//
		const char* OpName() const {
			return "GetBucketLocation";
		}
		
		//
		GetBucketLocation(
			const std::string& bucketName,
			const std::string& awsPublicKey,
			const std::string& awsPrivateKey,
			const s3::GetS3BucketLocationCallbackRef& callback)
			:
			mBucketName(bucketName),
			mAWSPublicKey(awsPublicKey),
			mAWSPrivateKey(awsPrivateKey),
			mCallback(callback),
			mAccessDeniedRetries(0) {
		}
		
		//
		ResultType AttemptOnce(const HermitPtr& h_) {

			s3::GetS3BucketLocationCallbackClass location;
			GetS3BucketLocation(h_, mBucketName, mAWSPublicKey, mAWSPrivateKey, location);
			if (location.mResult == s3::S3Result::kSuccess) {
				mLocation = location.mLocation;
			}
			return location.mResult;
		}

		// other operations may treat NoNetworkConnection as a state worth attempting
		// to retry, but since this is basically the first step in setting up an S3Bucket wrapper
		// it's not a great candidate for retry.
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
				mCallback.Call(result, mLocation);
				return;
			}
			NOTIFY_ERROR(h_, "GetBucketLocation: error result encountered:", (int)result);
			mCallback.Call(result, "");
		}
		
		//
		void RetriesExceeded(const HermitPtr& h_, const ResultType& result) {
			NOTIFY_ERROR(h_, "GetBucketLocation: maximum retries exceeded, most recent result:", (int)result);
			mCallback.Call(result, "");
		}

		//
		void Canceled() {
			mCallback.Call(s3::S3Result::kCanceled, "");
		}
	};

} // namespace
//
//
WithS3BucketStatus S3BucketImpl::Init(const HermitPtr& h_) {
	s3::GetS3BucketLocationCallbackClass location;
	GetBucketLocation getLocation(mBucketName, mAWSPublicKey, mAWSPrivateKey, location);
	s3::RetryClassT<GetBucketLocation> retry;
	retry.AttemptWithRetry(h_, getLocation);

	if (location.mResult == s3::S3Result::kCanceled) {
		return WithS3BucketStatus::kCancel;
	}
	if (location.mResult == s3::S3Result::kInvalidAccessKey) {
		return WithS3BucketStatus::kInvalidAccessKey;
	}
	if (location.mResult == s3::S3Result::kSignatureDoesNotMatch) {
		return WithS3BucketStatus::kSignatureDoesNotMatch;
	}
	if (location.mResult == s3::S3Result::k403AccessDenied) {
		return WithS3BucketStatus::kAccessDenied;
	}
	if (location.mResult == s3::S3Result::k404NoSuchBucket) {
		return WithS3BucketStatus::kNoSuchBucket;
	}
	if (location.mResult == s3::S3Result::kNoNetworkConnection) {
		return WithS3BucketStatus::kNoNetworkConnection;
	}
	if (location.mResult == s3::S3Result::kNetworkConnectionLost) {
		return WithS3BucketStatus::kNetworkConnectionLost;
	}
	if (location.mResult == s3::S3Result::kTimedOut) {
		return WithS3BucketStatus::kTimedOut;
	}
	if (location.mResult != s3::S3Result::kSuccess) {
		NOTIFY_ERROR(h_, "S3Bucket::Init: GetS3BucketLocation failed for bucket:", mBucketName);
		return WithS3BucketStatus::kError;
	}

	mAWSRegion = location.mLocation;

	s3::GenerateAWS4SigningKeyCallbackClass keyCallback;
	GenerateAWS4SigningKey(mAWSPrivateKey, mAWSRegion, keyCallback);
	mAWSSigningKey = keyCallback.mKey;
	mSigningKeyDateString = keyCallback.mDateString;
		
	return WithS3BucketStatus::kSuccess;
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
