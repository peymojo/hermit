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
#include "S3Bucket.h"

namespace hermit {
	namespace s3bucket {
		
		//
		s3::S3Result S3Bucket::ListObjects(const HermitPtr& h_,
										   const std::string& prefix,
										   s3::ObjectKeyReceiver& receiver) {
			NOTIFY_ERROR(h_, "S3Bucket::ListObjects unimplemented");
			return s3::S3Result::kError;
		}
		
		//
		void S3Bucket::GetObject(const HermitPtr& h_,
								 const std::string& inS3ObjectKey,
								 const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
								 const s3::S3CompletionBlockPtr& inCompletion) {
			NOTIFY_ERROR(h_, "S3Bucket::GetObject unimplemented");
			inCompletion->Call(h_, s3::S3Result::kError);
		}
		
		//
		void S3Bucket::GetObjectVersion(const HermitPtr& h_,
										const std::string& inS3ObjectKey,
										const std::string& inVersion,
										const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
										const s3::S3CompletionBlockPtr& inCompletion) {
			NOTIFY_ERROR(h_, "S3Bucket::GetObjectVersion unimplemented");
			inCompletion->Call(h_, s3::S3Result::kError);
		}
		
		//
		void S3Bucket::PutObject(const HermitPtr& h_,
								 const std::string& inS3ObjectKey,
								 const DataBuffer& inData,
								 const bool& inUseReducedRedundancyStorage,
								 const s3::PutS3ObjectCallbackRef& inCallback) {
			NOTIFY_ERROR(h_, "S3Bucket::PutObject unimplemented");
			inCallback.Call(s3::S3Result::kError, nullptr);
		}
		
		//
		s3::S3Result S3Bucket::DeleteObject(const HermitPtr& h_, const std::string& inObjectKey) {
			NOTIFY_ERROR(h_, "S3Bucket::DeleteObject unimplemented");
			return s3::S3Result::kError;
		}
		
		//
		void S3Bucket::IsVersioningEnabled(const HermitPtr& h_,
										   const s3::S3GetBucketVersioningCallbackRef& inCallback) {
			NOTIFY_ERROR(h_, "S3Bucket::IsVersioningEnabled unimplemented");
			inCallback.Call(s3::S3Result::kError, s3::S3BucketVersioningStatus::kUnknown);
		}
		
	} // namespace s3bucket
} // namespace hermit
