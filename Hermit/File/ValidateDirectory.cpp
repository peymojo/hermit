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
#include "FileExists.h"
#include "PathIsDirectory.h"
#include "ValidateDirectory.h"

namespace hermit {
	namespace file {
		
		//
		bool ValidateDirectory(const HermitPtr& h_, const FilePathPtr& filePath) {
			bool exists = false;
			if (!FileExists(h_, filePath, exists)) {
				NOTIFY_ERROR(h_, "FileExists failed for path:", filePath);
				return false;
			}
			if (!exists) {
				NOTIFY_ERROR(h_, "Folder path not found:", filePath);
				return false;
			}

			bool isDirectory = false;
			auto status = PathIsDirectory(h_, filePath, isDirectory);
			if (status != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "PathIsDirectory failed for path:", filePath);
				return false;
			}
			if (!isDirectory) {
				NOTIFY_ERROR(h_, "Folder path not a directory?", filePath);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit
