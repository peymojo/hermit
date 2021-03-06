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

#ifndef StreamDataFunction_h
#define StreamDataFunction_h

#include "AsyncFunction.h"
#include "DataBuffer.h"
#include "Hermit.h"

namespace hermit {
	
	//
	enum class StreamDataResult {
		kUnknown,
		kSuccess,
		kCanceled,
		kDiskFull,
		kFileNotFound,
		kError
	};

	//
	DEFINE_ASYNC_FUNCTION_2A(DataCompletion,
							 HermitPtr,
							 StreamDataResult);				// result
	
	//
	DEFINE_ASYNC_FUNCTION_4A(DataReceiver,
							 HermitPtr,
							 DataBuffer,					// data
							 bool,							// isEndOfData
							 DataCompletionPtr);			// completion
	
	//
	DEFINE_ASYNC_FUNCTION_3A(DataProvider,
							 HermitPtr,
							 DataReceiverPtr,				// receiver
							 DataCompletionPtr);			// completion
	
	//
	DEFINE_ASYNC_FUNCTION_3A(DataProviderClient,
							 HermitPtr,
							 DataProviderPtr,				// provider
							 DataCompletionPtr);			// completion
	
} // namespace hermit

#endif
