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

#ifndef FileSystemCopy_h
#define FileSystemCopy_h

#include "Hermit/Foundation/Callback.h"
#include "FilePath.h"

namespace hermit {
namespace file {

//
//
enum FileSystemCopyStatus
{
	kFileSystemCopyStatus_Unknown,
	kFileSystemCopyStatus_Success,
	kFileSystemCopyStatus_SourceNotFound,
	kFileSystemCopyStatus_TargetAlreadyExists,
	kFileSystemCopyStatus_Error,
	kFileSystemCopyStatus_VerifyFailed,
	kFileSystemCopyStatus_VerifyError
};

//
//
DEFINE_CALLBACK_3A(
	FileSystemCopyCallback,
	FileSystemCopyStatus,	// inStatus
	FilePathPtr,			// inSourcePath
	FilePathPtr);			// inDestPath

//
//
void FileSystemCopy(
	const FilePathPtr& inSourcePath,
	const FilePathPtr& inDestPath,
	const FileSystemCopyCallbackRef& inCallback);

} // namespace file
} // namespace hermit

#endif
