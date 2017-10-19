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

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace s3 {
		
		//
		//
		enum GetS3BucketACLXMLResult
		{
			kGetS3BucketACLXMLResult_Unknown,
			kGetS3BucketACLXMLResult_Success,
			kGetS3BucketACLXMLResult_NoSuchBucket,
			kGetS3BucketACLXMLResult_Error
		};
		
		//
		//
		DEFINE_CALLBACK_2A(
						   GetS3BucketACLXMLCallback,
						   GetS3BucketACLXMLResult,		// inResult
						   std::string);					// inACLXML
		
		//
		//
		template <typename T>
		class GetS3BucketACLXMLCallbackClassT
		:
		public GetS3BucketACLXMLCallback
		{
		public:
			//
			//
			GetS3BucketACLXMLCallbackClassT()
			:
			mResult(kGetS3BucketACLXMLResult_Unknown)
			{
			}
			
			//
			//
			bool Function(const GetS3BucketACLXMLResult& inResult,
						  const std::string& inACLXML)
			{
				mResult = inResult;
				if (inResult == kGetS3BucketACLXMLResult_Success)
				{
					mACLXML = inACLXML;
				}
				return true;
			}
			
			//
			//
			GetS3BucketACLXMLResult mResult;
			T mACLXML;
		};
		
		//
		//
		void GetS3BucketACLXML(const HermitPtr& h_,
							   const std::string& inBucketName,
							   const std::string& inS3PublicKey,
							   const std::string& inS3PrivateKey,
							   const GetS3BucketACLXMLCallbackRef& inCallback);
		
	} // namespace s3
} // namespace hermit

#endif 
