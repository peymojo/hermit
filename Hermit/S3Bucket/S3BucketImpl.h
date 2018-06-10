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

#ifndef S3BucketImpl_h
#define S3BucketImpl_h

#include <string>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/ThreadLock.h"
#include "S3Bucket.h"
#include "WithS3Bucket.h"

namespace hermit {
	namespace s3bucket {
		namespace impl {
			
            //
            DEFINE_ASYNC_FUNCTION_2A(InitS3BucketCompletion, HermitPtr, WithS3BucketStatus);
            
			//
			class S3BucketImpl : public S3Bucket, public std::enable_shared_from_this<S3BucketImpl> {
			public:
				//
				static const int kMaxRetries = 8;
				
				//
				S3BucketImpl(const std::string& awsPublicKey,
							 const std::string& awsPrivateKey,
							 const std::string& bucketName);
				
                //
                void Init(const HermitPtr& h_, const InitS3BucketCompletionPtr& completion);

                //
				virtual void ListObjects(const HermitPtr& h_,
                                         const std::string& prefix,
                                         const s3::ObjectKeyReceiverPtr& receiver,
                                         const s3::S3CompletionBlockPtr& completion) override;
				
				//
				virtual void GetObject(const HermitPtr& h_,
									   const std::string& inS3ObjectKey,
									   const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
									   const s3::S3CompletionBlockPtr& inCompletion) override;
				
				//
				virtual void GetObjectVersion(const HermitPtr& h_,
											  const std::string& inS3ObjectKey,
											  const std::string& inVersion,
											  const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
											  const s3::S3CompletionBlockPtr& inCompletion) override;
				
				//
				virtual void PutObject(const HermitPtr& h_,
									   const std::string& inS3ObjectKey,
									   const SharedBufferPtr& inData,
									   const bool& inUseReducedRedundancyStorage,
									   const s3::PutS3ObjectCompletionPtr& inCompletion) override;
				
				//
				virtual void DeleteObject(const HermitPtr& h_,
                                          const std::string& objectKey,
                                          const s3::S3CompletionBlockPtr& completion) override;
				
				//
				virtual void IsVersioningEnabled(const HermitPtr& h_, const s3::S3GetBucketVersioningCompletionPtr& inCompletion) override;
				
                //
                void PutMultipartObjectToS3Bucket(const HermitPtr& h_,
                                                  const std::string& s3ObjectKey,
                                                  const SharedBufferPtr& data,
                                                  const bool& useReducedRedundancyStorage, // TODO: currently ignored
                                                  const s3::PutS3ObjectCompletionPtr& completion);
                
				//
				void RefreshSigningKeyIfNeeded();
				
				//
				http::HTTPSessionPtr mHTTPSession;
				std::string mAWSPublicKey;
				std::string mAWSPrivateKey;
				std::string mAWSSigningKey;
				std::string mAWSRegion;
				std::string mBucketName;
				std::string mSigningKeyDateString;
				ThreadLock mLock;
			};
			
		} // namespace impl
	} // namespace s3bucket
} // namespace hermit

#endif 
