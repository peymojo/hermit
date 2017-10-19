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

#ifndef S3Result_h
#define S3Result_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace s3 {
		
		enum class S3Result {
			kUnknown,
			kSuccess,
			kCanceled,
			kAuthorizationHeaderMalformed,
			kInvalidAccessKey,
			kSignatureDoesNotMatch,
			k301PermanentRedirect,
			k307TemporaryRedirect,
			k400BadRequest,
			k403AccessDenied,
			k404EntityNotFound,
			k404NoSuchBucket,
			k500InternalServerError,
			k503ServiceUnavailable,
			kS3InternalError,
			kChecksumMismatch,
			kHostNotFound,
			kNetworkConnectionLost,
			kNoNetworkConnection,
			kTimedOut,
			kError
		};

		//
		DEFINE_ASYNC_FUNCTION_2A(S3CompletionBlock, HermitPtr, S3Result);

	} // namespace
} // namespace

#endif
