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

#include "Hermit/Foundation/Notification.h"
#include "CreateDirectoryIfNeeded.h"
#include "FileExists.h"
#include "GetFilePathParent.h"
#include "PathIsDirectory.h"
#include "CreateDirectoryParentChain.h"

namespace hermit {
	namespace file {

		//
		CreateDirectoryParentChainResult CreateDirectoryParentChain(const HermitPtr& h_, const FilePathPtr& directorypPath) {
			bool exists = false;
			if (!FileExists(h_, directorypPath, exists)) {
				NOTIFY_ERROR(h_, "FileExists failed for path:", directorypPath);
				return CreateDirectoryParentChainResult::kError;
			}
			
			if (exists) {
				bool isDirectory = false;
				auto status = PathIsDirectory(h_, directorypPath, isDirectory);
				if (status != PathIsDirectoryStatus::kSuccess) {
					NOTIFY_ERROR(h_, "PathIsDirectory failed for path:", directorypPath);
					return CreateDirectoryParentChainResult::kError;
				}
				if (!isDirectory) {
					return CreateDirectoryParentChainResult::kConflictAtPath;
				}
				return CreateDirectoryParentChainResult::kSuccess;
			}

			FilePathPtr parentPath;
			GetFilePathParent(h_, directorypPath, parentPath);
			if (parentPath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathParent failed for path:", directorypPath);
				return CreateDirectoryParentChainResult::kError;
			}

			auto parentResult = CreateDirectoryParentChain(h_, parentPath);
			if (parentResult != CreateDirectoryParentChainResult::kSuccess) {
				return parentResult;
			}

			auto result = CreateDirectoryIfNeeded(h_, directorypPath);
			if (result.first == kCreateDirectoryIfNeededStatus_Success) {
				return CreateDirectoryParentChainResult::kSuccess;
			}
			if (result.first == kCreateDirectoryIfNeededStatus_ConflictAtPath) {
				return CreateDirectoryParentChainResult::kConflictAtPath;
			}
			if (result.first == kCreateDirectoryIfNeededStatus_DiskFull) {
				return CreateDirectoryParentChainResult::kDiskFull;
			}
			return CreateDirectoryParentChainResult::kError;
		}
		
	} // namespace file
} // namespace hermit
