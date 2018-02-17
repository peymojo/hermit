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

#include <sys/stat.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "GetFileType.h"

namespace hermit {
	namespace file {
		
		//
		GetFileTypeStatus GetFileType(const HermitPtr& h_, const FilePathPtr& filePath, FileType& outType) {
			std::string itemPathUTF8;
			FilePathToCocoaPathString(h_, filePath, itemPathUTF8);

			struct stat s;
			int err = lstat(itemPathUTF8.c_str(), &s);
			if (err != 0) {
				NOTIFY_ERROR(h_, "lstat failed for path:", itemPathUTF8, "err:", err);
				return GetFileTypeStatus::kError;
			}

			FileType fileType = FileType::kUnknown;
			if (S_ISREG(s.st_mode)) {
				fileType = FileType::kFile;
			}
			else if (S_ISDIR(s.st_mode)) {
				fileType = FileType::kDirectory;
			}
			else if (S_ISLNK(s.st_mode)) {
				fileType = FileType::kSymbolicLink;
			}
			else if (S_ISBLK(s.st_mode) || S_ISCHR(s.st_mode) || S_ISFIFO(s.st_mode) || S_ISSOCK(s.st_mode)) {
				fileType = FileType::kDevice;
			}
			else {
				NOTIFY_ERROR(h_, "Unrecognized file type:", itemPathUTF8);
				return GetFileTypeStatus::kError;
			}

			outType = fileType;
			return GetFileTypeStatus::kSuccess;
		}
		
	} // namespace file
} // namespace hermit
