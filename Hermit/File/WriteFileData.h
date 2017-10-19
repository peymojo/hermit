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

#ifndef WriteFileData_h
#define WriteFileData_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum class WriteFileDataResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kDiskFull,
			kError
		};
		//
		DEFINE_ASYNC_FUNCTION_2A(WriteFileDataCompletion, HermitPtr, WriteFileDataResult);
		
		//
		void WriteFileData(const HermitPtr& h_,
						   const FilePathPtr& filePath,
						   const DataBuffer& data,
						   const WriteFileDataCompletionPtr& completion);
		
	} // namespace file
} // namespace hermit

#endif

