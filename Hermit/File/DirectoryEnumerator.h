//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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

#ifndef DirectoryEnumerator_h
#define DirectoryEnumerator_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "FileType.h"

namespace hermit {
	namespace file {
		
		//
		enum class GetNextDirectoryItemResult {
			kUnknown,
			kSuccess,
			kNoMoreItems,
			kCanceled,
			kNotADirectory,
			kPermissionDenied,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_4A(GetNextDirectoryItemCallback,
								 HermitPtr,
								 GetNextDirectoryItemResult,
								 FilePathPtr,
								 FileType);

		//
		DEFINE_ASYNC_FUNCTION_2A(GetNextDirectoryItemFunction,
								 HermitPtr,
								 GetNextDirectoryItemCallbackPtr);

		//
		enum class NewDirectoryEnumeratorResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kNotADirectory,
			kPermissionDenied,
			kError
		};

		//
		DEFINE_ASYNC_FUNCTION_3A(NewDirectoryEnumeratorCallback,
								 HermitPtr,
								 NewDirectoryEnumeratorResult,
								 GetNextDirectoryItemFunctionPtr);

		//
		void NewDirectoryEnumerator(const HermitPtr& h_,
									const FilePathPtr& directoryPath,
									const NewDirectoryEnumeratorCallbackPtr callback);
		
	} // namespace file
} // namespace hermit

#endif /* DirectoryEnumerator_h */
