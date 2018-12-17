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

#include <string>
#include "Hermit/Foundation/Notification.h"
#include "CreateDirectory.h"
#include "FileExists.h"
#include "GetFilePathUTF8String.h"
#include "GetFilePathParent.h"
#include "PathIsDirectory.h"
#include "CreateDirectoryIfNeeded.h"

namespace hermit {
	namespace file {
		
		//
		CreateDirectoryIfNeededResult CreateDirectoryIfNeeded(const HermitPtr& h_, const FilePathPtr& path) {
			if (path == nullptr) {
				NOTIFY_ERROR(h_, "path is null");
				return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Error, false);
			}
			
			bool exists = false;
			if (!FileExists(h_, path, exists)) {
				NOTIFY_ERROR(h_, "FileExists failed for path:", path);
				return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Error, false);
			}
			
			if (exists) {
				bool isDirectory = false;
				auto status = PathIsDirectory(h_, path, isDirectory);
				if (status != PathIsDirectoryStatus::kSuccess) {
					NOTIFY_ERROR(h_, "PathIsDirectory failed for path:", path);
					return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Error, false);
				}
				if (isDirectory) {
					return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Success, false);
				}
				return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_ConflictAtPath, false);
			}

			FilePathPtr parentPath;
			GetFilePathParent(h_, path, parentPath);
			if (parentPath != nullptr) {
				std::string filePathUTF8;
				GetFilePathUTF8String(h_, parentPath, filePathUTF8);
				if (filePathUTF8 == "/Volumes") {
					NOTIFY_ERROR(h_, "Will not attempt to create directory under /Volumes:", path);
					return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Error, false);
				}
			}
				
			auto result = CreateDirectory(h_, path);
			if (result == CreateDirectoryResult::kDiskFull) {
				return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_DiskFull, false);
			}
			if (result != CreateDirectoryResult::kSuccess) {
				NOTIFY_ERROR(h_, "CreateDirectory failed for path:", path);
				return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Error, false);
			}
			return CreateDirectoryIfNeededResult(kCreateDirectoryIfNeededStatus_Success, true);
		}
		
	} // namespace file
} // namespace hermit
