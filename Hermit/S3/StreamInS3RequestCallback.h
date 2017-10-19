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

#ifndef StreamInS3RequestCallback_h
#define StreamInS3RequestCallback_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/EnumerateStringValuesFunction.h"
#include "S3Result.h"

namespace hermit {
namespace s3 {

//
//
DEFINE_CALLBACK_2A(
	StreamInS3RequestCallback,
	S3Result,								// inResult
	EnumerateStringValuesFunctionRef);		// inParamFunction

} // namespace s3
} // namespace hermit

#endif
