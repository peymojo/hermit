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
#include "PathIsDirectory.h"
#include "PathIsSymbolicLink.h"
#include "GetFileType.h"

namespace hermit {
	namespace file {
		
		//
		GetFileTypeStatus GetFileType(const HermitPtr& h_, const FilePathPtr& filePath, FileType& outType) {
		
			FileType fileType = FileType::kUnknown;
			
			//	Check for symbolic link first, as a link to a directory might also identify as a directory.
			PathIsSymbolicLinkCallbackClass isLinkCallback;
			PathIsSymbolicLink(h_, filePath, isLinkCallback);
			if (!isLinkCallback.mSuccess) {
				NOTIFY_ERROR(h_, "GetFileType: PathIsSymbolicLink failed for path:", filePath);
				return GetFileTypeStatus::kError;
			}
			if (isLinkCallback.mIsLink) {
				fileType = FileType::kSymbolicLink;
			}
			else {
				bool isDirectory = false;
				auto status = PathIsDirectory(h_, filePath, isDirectory);
				if (status != PathIsDirectoryStatus::kSuccess) {
					NOTIFY_ERROR(h_, "GetFileType: PathIsDirectory failed for path:", filePath);
					return GetFileTypeStatus::kError;
				}
				if (isDirectory) {
					fileType = FileType::kDirectory;
				}
				else {
					fileType = FileType::kFile;
				}
			}
			outType = fileType;
			return GetFileTypeStatus::kSuccess;
		}
		
	} // namespace file
} // namespace hermit
