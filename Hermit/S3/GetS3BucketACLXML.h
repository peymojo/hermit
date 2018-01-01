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

#ifndef GetS3BucketACLXML_h
#define GetS3BucketACLXML_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace s3 {
		
		//
		enum class GetS3BucketACLXMLResult {
			kUnknown,
			kSuccess,
			kNoSuchBucket,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(GetS3BucketACLXMLCompletion,
								 HermitPtr,
								 GetS3BucketACLXMLResult,		// result
								 std::string);					// aclXml
		
		//
		void GetS3BucketACLXML(const HermitPtr& h_,
							   const std::string& bucketName,
							   const std::string& s3PublicKey,
							   const std::string& s3PrivateKey,
							   const GetS3BucketACLXMLCompletionPtr& completion);
		
	} // namespace s3
} // namespace hermit

#endif 
