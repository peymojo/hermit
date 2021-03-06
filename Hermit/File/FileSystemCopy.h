//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef FileSystemCopy_h
#define FileSystemCopy_h

#include <memory>
#include "Hermit/Foundation/AsyncFunction.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum class FileSystemCopyResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kStoppedViaUpdateCallback,
			kSourceNotFound,
			kTargetAlreadyExists,
			kError
		};

		//
		class FileSystemCopyIntermediateUpdateCallback {
		public:
			//
			virtual ~FileSystemCopyIntermediateUpdateCallback() = default;
			
			//
			virtual bool OnUpdate(const HermitPtr& h_, const FileSystemCopyResult& result, const FilePathPtr& source, const FilePathPtr& dest) = 0;
		};
		typedef std::shared_ptr<FileSystemCopyIntermediateUpdateCallback> FileSystemCopyIntermediateUpdateCallbackPtr;
		
		//
		DEFINE_ASYNC_FUNCTION_2A(FileSystemCopyCompletion,
								 HermitPtr,
								 FileSystemCopyResult);
		
		//
		void FileSystemCopy(const HermitPtr& h_,
							const FilePathPtr& sourcePath,
							const FilePathPtr& destPath,
							const FileSystemCopyIntermediateUpdateCallbackPtr& updateCallback,
							const FileSystemCopyCompletionPtr& completion);
		
	} // namespace file
} // namespace hermit

#endif
