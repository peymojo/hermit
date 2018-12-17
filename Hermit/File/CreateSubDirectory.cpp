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
#include "AppendToFilePath.h"
#include "CreateDirectoryIfNeeded.h"
#include "CreateSubDirectory.h"

namespace hermit {
	namespace file {
		
		//
		bool CreateSubDirectory(const HermitPtr& h_, const FilePathPtr& parent, const std::string& directoryName, FilePathPtr& outSubDirectory) {
			FilePathPtr dirPath;
			AppendToFilePath(h_, parent, directoryName, dirPath);
			if (dirPath == nullptr) {
				NOTIFY_ERROR(h_, "AppendToFilePathFailed for directoryName:", directoryName);
				return false;
			}
			
			bool wasCreated = false;
			auto result = CreateDirectoryIfNeeded(h_, dirPath, wasCreated);
			if (result != CreateDirectoryIfNeededResult::kSuccess) {
				NOTIFY_ERROR(h_, "CreateDirectoryIfNeeded for dirPath:", dirPath);
				return false;
			}
			outSubDirectory = dirPath;
			return true;
		}
		
	} // namespace file
} // namespace hermit
