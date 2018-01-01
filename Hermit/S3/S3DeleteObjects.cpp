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

#include <string>
#include "Hermit/Foundation/Notification.h"
#include "S3DeleteObject.h"
#include "S3DeleteObjectVersion.h"
#include "S3ListObjectsWithVersions.h"
#include "S3DeleteObjects.h"

namespace hermit {
	namespace s3 {
		namespace S3DeleteObjects_Impl {

#if 000
			//
			class S3ListObjectsWithVersionsClass : public S3ListObjectsWithVersionsCompletion {
			public:
				//
				//
				S3ListObjectsWithVersionsClass(const HermitPtr& h_,
											   const std::string& inAWSPublicKey,
											   const std::string& inAWSSigningKey,
											   const uint64_t& inAWSSigningKeySize,
											   const std::string& inAWSRegion,
											   const std::string& inBucketName,
											   const S3DeleteObjectsCallbackRef& inCallback)
				:
				mH_(h_),
				mBucketName(inBucketName),
				mAWSPublicKey(inAWSPublicKey),
				mAWSSigingKey(inAWSSigningKey, inAWSSigningKeySize),
				mAWSRegion(inAWSRegion),
				mCallback(inCallback) {
				}
				
				//
				bool Function(const S3ListObjectsWithVersionsStatus& inStatus,
							  const std::string& inObjectKey,
							  const std::string& inVersion,
							  const bool& inIsDeleteMarker) {
					if (inStatus == kS3ListObjectsWithVersionsStatus_Success) {
						if (inVersion == "null") {
							auto result = S3DeleteObject(mH_,
														 mAWSPublicKey,
														 mAWSSigingKey,
														 mAWSRegion,
														 mBucketName,
														 inObjectKey);
							
							if (!mCallback.Call(mBucketName, inObjectKey, "", (result == S3Result::kSuccess))) {
								return false;
							}
						}
						else {
							bool success = S3DeleteObjectVersion(mH_,
																 mAWSPublicKey,
																 mAWSSigingKey.data(),
																 mAWSSigingKey.size(),
																 mAWSRegion,
																 mBucketName,
																 inObjectKey,
																 inVersion);
							return mCallback.Call(mBucketName, inObjectKey, inVersion, success);
						}
					}
					return true;
				}
				
			private:
				//
				HermitPtr mH_;
				std::string mAWSPublicKey;
				std::string mAWSSigingKey;
				std::string mAWSRegion;
				std::string mBucketName;
				S3DeletedObjectReporterPtr mDeletedObjectReporter;
				S3CompletionBlockPtr mCompletion;
			};
#endif
			
		} // namespace S3DeleteObjects_Impl
		using namespace S3DeleteObjects_Impl;
		
		//
		void S3DeleteObjects(const HermitPtr& h_,
							 const std::string& awsPublicKey,
							 const std::string& awsSigningKey,
							 const std::string& awsRegion,
							 const std::string& bucketName,
							 const std::string& pathPrefix,
							 const S3DeletedObjectReporterPtr& deletedObjectReporter,
							 const S3CompletionBlockPtr& completion) {
			
			NOTIFY_ERROR(h_, "S3DeleteObjects: not implemented");
			completion->Call(h_, S3Result::kError);
		
			// needs to be updated for async processing and underlying calls need to be updated
#if 000
			auto lister = std::make_shared<S3ListObjectsWithVersionsClass>(h_,
																		   awsPublicKey,
																		   awsSigningKey,
																		   awsRegion,
																		   bucketName,
																		   deletedObjectReporter,
																		   completion);
			
			S3ListObjectsWithVersions(h_,
									  awsPublicKey,
									  awsSigningKey,
									  awsRegion,
									  bucketName,
									  pathPrefix,
									  lister);
#endif
		}
		
	} // namespace s3
} // namespace hermit
