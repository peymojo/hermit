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

#ifndef S3GetBucketVersioning_h
#define S3GetBucketVersioning_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "S3Result.h"

namespace hermit {
	namespace s3 {
		
		//
		//
		enum class S3BucketVersioningStatus {
			kUnknown,
			kNeverEnabled,
			kOn,
			kSuspended
		};
		
		//
		//
		DEFINE_CALLBACK_2A(
						   S3GetBucketVersioningCallback,
						   S3Result,						// inResult
						   S3BucketVersioningStatus);		// inStatus
		
		//
		//
		class S3GetBucketVersioningCallbackClass
		:
		public S3GetBucketVersioningCallback
		{
		public:
			//
			//
			S3GetBucketVersioningCallbackClass()
			:
			mResult(S3Result::kUnknown),
			mStatus(S3BucketVersioningStatus::kUnknown)
			{
			}
			
			//
			//
			bool Function(const S3Result& inResult, const S3BucketVersioningStatus& inStatus) {
				mResult = inResult;
				if (inResult == S3Result::kSuccess) {
					mStatus = inStatus;
				}
				return true;
			}
			
			//
			//
			S3Result mResult;
			S3BucketVersioningStatus mStatus;
		};
		
		//
		void S3GetBucketVersioning(const HermitPtr& h_,
								   const std::string& inBucketName,
								   const std::string& inS3PublicKey,
								   const std::string& inS3PrivateKey,
								   const S3GetBucketVersioningCallbackRef& inCallback);
		
	} // namespace s3
} // namespace hermit

#endif

