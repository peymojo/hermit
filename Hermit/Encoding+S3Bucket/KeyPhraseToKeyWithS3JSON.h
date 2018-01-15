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

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/S3Bucket/S3Bucket.h"

namespace hermit {
	namespace encoding_s3bucket {
		
		//
		enum class KeyPhraseToKeyWithS3JSONStatus {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(KeyPhraseToKeyWithS3JSONCompletion,
                                 HermitPtr,
                                 KeyPhraseToKeyWithS3JSONStatus,		// inStatus
                                 std::string);					    	// inAESKey
				
		//
		void KeyPhraseToKeyWithS3JSON(const HermitPtr& h_,
									  const std::string& keyPhrase,
									  const s3bucket::S3BucketPtr& s3Bucket,
									  const std::string& s3BasePath,
									  const std::string& keyJSONName,
									  const KeyPhraseToKeyWithS3JSONCompletionPtr& completion);
		
	} // namespace encoding_s3bucket
} // namespace hermit

#endif 
