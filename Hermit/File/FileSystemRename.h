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

#ifndef FileSystemRename_h
#define FileSystemRename_h

#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum FileSystemRenameStatus {
			kFileSystemRenameStatus_Unknown,
			kFileSystemRenameStatus_Success,
			kFileSystemRenameStatus_SourceNotFound,
			kFileSystemRenameStatus_TargetAlreadyExists,
			kFileSystemRenameStatus_Error
		};
		
		//
		FileSystemRenameStatus FileSystemRename(const FilePathPtr& inFilePath,
												const std::string& inNewFilename,
												const bool& inOverwriteExisting);
		
	} // namespace file
} // namespace hermit

#endif 

