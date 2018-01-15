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

#ifndef S3Bucket_h
#define S3Bucket_h

#include "Hermit/Foundation/Hermit.h"
#include "Hermit/S3/GetS3Object.h"
#include "Hermit/S3/GetS3ObjectWithVersion.h"
#include "Hermit/S3/PutS3Object.h"
#include "Hermit/S3/S3DeleteObject.h"
#include "Hermit/S3/S3GetBucketVersioning.h"
#include "Hermit/S3/S3ListObjects.h"

namespace hermit {
	namespace s3bucket {
		
		//
		class S3Bucket {
		public:
			//
			virtual void ListObjects(const HermitPtr& h_,
                                     const std::string& prefix,
                                     const s3::ObjectKeyReceiverPtr& receiver,
                                     const s3::S3CompletionBlockPtr& completion);
			
			//
			virtual void GetObject(const HermitPtr& h_,
								   const std::string& inS3ObjectKey,
								   const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
								   const s3::S3CompletionBlockPtr& inCompletion);
			
			//
			virtual void GetObjectVersion(const HermitPtr& h_,
										  const std::string& inS3ObjectKey,
										  const std::string& inVersion,
										  const s3::GetS3ObjectResponseBlockPtr& inResponseBlock,
										  const s3::S3CompletionBlockPtr& inCompletion);
			
			//
			virtual void PutObject(const HermitPtr& h_,
								   const std::string& inS3ObjectKey,
								   const SharedBufferPtr& inData,
								   const bool& inUseReducedRedundancyStorage,
								   const s3::PutS3ObjectCompletionPtr& inCompletion);
			
			//
			virtual void DeleteObject(const HermitPtr& h_,
                                      const std::string& objectKey,
                                      const s3::S3CompletionBlockPtr& completion);
			
			//
			virtual void IsVersioningEnabled(const HermitPtr& h_,
											 const s3::S3GetBucketVersioningCompletionPtr& inCompletion);
			
		protected:
			//
			virtual ~S3Bucket() = default;
		};
		typedef std::shared_ptr<S3Bucket> S3BucketPtr;
		
	} // namespace s3bucket
} // namespace hermit

#endif
