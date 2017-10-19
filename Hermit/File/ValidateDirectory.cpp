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
		bool ValidateDirectory(const HermitPtr& h_, const FilePathPtr& inFilePath) {
			FileExistsCallbackClass existsCallback;
			FileExists(h_, inFilePath, existsCallback);
			if (!existsCallback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateDirectory: FileExists failed for path:", inFilePath);
				return false;
			}
			if (!existsCallback.mExists) {
				NOTIFY_ERROR(h_, "ValidateDirectory: Folder path not found:", inFilePath);
				return false;
			}

			bool isDirectory = false;
			auto status = PathIsDirectory(h_, inFilePath, isDirectory);
			if (status != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "ValidateDirectory: PathIsDirectory failed for path:", inFilePath);
				return false;
			}
			if (!isDirectory) {
				NOTIFY_ERROR(h_, "ValidateDirectory: Folder path not a directory?", inFilePath);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit
