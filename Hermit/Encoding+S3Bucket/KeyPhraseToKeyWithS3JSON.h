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

#ifndef KeyPhraseToKeyWithS3JSON_h
#define KeyPhraseToKeyWithS3JSON_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/S3Bucket/S3Bucket.h"

namespace hermit {
	namespace encoding_s3bucket {
		
		//
		//
		enum KeyPhraseToKeyWithS3JSONStatus
		{
			kKeyPhraseToKeyWithS3JSONStatus_Unknown,
			kKeyPhraseToKeyWithS3JSONStatus_Success,
			kKeyPhraseToKeyWithS3JSONStatus_Canceled,
			kKeyPhraseToKeyWithS3JSONStatus_Error
		};
		
		//
		//
		DEFINE_CALLBACK_2A(KeyPhraseToKeyWithS3JSONCallback,
						   KeyPhraseToKeyWithS3JSONStatus,		// inStatus
						   std::string);						// inAESKey
		
		//
		//
		class KeyPhraseToKeyWithS3JSONCallbackClass
		:
		public KeyPhraseToKeyWithS3JSONCallback
		{
		public:
			//
			//
			KeyPhraseToKeyWithS3JSONCallbackClass()
			:
			mStatus(kKeyPhraseToKeyWithS3JSONStatus_Unknown)
			{
			}
			
			//
			//
			bool Function(
						  const KeyPhraseToKeyWithS3JSONStatus& inStatus,
						  const std::string& inAESKey)
			{
				mStatus = inStatus;
				if (inStatus == kKeyPhraseToKeyWithS3JSONStatus_Success)
				{
					mAESKey = inAESKey;
				}
				return true;
			}
			
			//
			//
			KeyPhraseToKeyWithS3JSONStatus mStatus;
			std::string mAESKey;
		};
		
		//
		//
		void KeyPhraseToKeyWithS3JSON(const HermitPtr& h_,
									  const std::string& inKeyPhrase,
									  const s3bucket::S3BucketPtr& inS3Bucket,
									  const std::string& inS3BasePath,
									  const std::string& inKeyJSONName,
									  const KeyPhraseToKeyWithS3JSONCallbackRef& inCallback);
		
	} // namespace encoding_s3bucket
} // namespace hermit

#endif 
