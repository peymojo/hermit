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
#include "S3BucketImpl.h"
#include "WithS3Bucket.h"

namespace hermit {
	namespace s3bucket {
        namespace WithS3Bucket_Impl {
            
            //
            typedef std::shared_ptr<impl::S3BucketImpl> S3BucketImplPtr;

            //
            class InitCompletion : public impl::InitS3BucketCompletion {
            public:
                //
                InitCompletion(const S3BucketImplPtr& bucket, const WithS3BucketCompletionPtr& completion) :
                mBucket(bucket),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const WithS3BucketStatus& status) override {
                    if (status == WithS3BucketStatus::kCancel) {
                        mCompletion->Call(h_, WithS3BucketStatus::kCancel, nullptr);
                        return;
                    }
                    if (status != WithS3BucketStatus::kSuccess) {
                        if (status == WithS3BucketStatus::kError) {
                            NOTIFY_ERROR(h_, "WithS3Bucket: bucket->Init returned WithS3BucketStatus::kError.");
                        }
                        mCompletion->Call(h_, status, nullptr);
                    }
                    mCompletion->Call(h_, status, mBucket);
                }
                
                //
                S3BucketImplPtr mBucket;
                WithS3BucketCompletionPtr mCompletion;
            };
        
        } // namespace WithS3Bucket_Impl
        using namespace WithS3Bucket_Impl;
		
		//
		void WithS3Bucket(const HermitPtr& h_,
                          const std::string& bucketName,
                          const std::string& awsPublicKey,
                          const std::string& awsPrivateKey,
                          const WithS3BucketCompletionPtr& completion) {
			if (bucketName.empty()) {
				NOTIFY_ERROR(h_, "WithS3Bucket: bucketName is empty.");
				completion->Call(h_, WithS3BucketStatus::kError, nullptr);
				return;
			}
			
			auto bucket = std::make_shared<impl::S3BucketImpl>(awsPublicKey, awsPrivateKey, bucketName);
            auto initCompletion = std::make_shared<InitCompletion>(bucket, completion);
            bucket->Init(h_, initCompletion);
		}
		
	} // namespace s3bucket
} // namespace hermit
