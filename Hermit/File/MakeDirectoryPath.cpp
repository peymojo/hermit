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
#include "ValidateDirectory.h"
#include "MakeDirectoryPath.h"

namespace hermit {
	namespace file {
		
		//
		void MakeDirectoryPath(const HermitPtr& h_,
							   const FilePathPtr& inParentFilePath,
							   const std::string& inDirectoryName,
							   const bool& inCreateIfMissing,
							   const MakeDirectoryPathCallbackRef& inCallback) {
			
			FilePathPtr subDirPath;
			AppendToFilePath(h_, inParentFilePath, inDirectoryName, subDirPath);
			if (subDirPath == 0) {
				NOTIFY_ERROR(h_, "MakeDirectoryPath: AppendToFilePath failed for base path:", inParentFilePath);
				inCallback.Call(false, 0);
				return;
			}
			
			if (inCreateIfMissing) {
				bool wasCreated = false;
				auto result = CreateDirectoryIfNeeded(h_, subDirPath, wasCreated);
				if (result != CreateDirectoryIfNeededResult::kSuccess) {
					NOTIFY_ERROR(h_,"MakeDirectoryPath(): CreateDirectoryIfNeeded failed for path:", subDirPath);
					inCallback.Call(false, 0);
					return;
				}
			}
			if (!ValidateDirectory(h_, subDirPath)) {
				NOTIFY_ERROR(h_, "MakeDirectoryPath(): ValidateDirectory failed for path:", subDirPath);
				inCallback.Call(false, 0);
				return;
			}
			inCallback.Call(true, subDirPath);
		}
		
	} // namespace file
} // namespace hermit
