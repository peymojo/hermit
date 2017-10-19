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
		void CreateSubDirectory(const HermitPtr& h_,
								const FilePathPtr& inBasePath,
								const std::string& inDirectoryName,
								const CreateSubDirectoryCallbackRef& inCallback) {
			FilePathPtr dirPath;
			AppendToFilePath(h_, inBasePath, inDirectoryName, dirPath);
			if (dirPath == nullptr) {
				NOTIFY_ERROR(h_, "CreateSubDirectory: AppendToFilePathFailed for directoryName:", inDirectoryName);
				inCallback.Call(false, nullptr);
				return;
			}
			
			auto result = CreateDirectoryIfNeeded(h_, dirPath);
			if (result.first != kCreateDirectoryIfNeededStatus_Success) {
				inCallback.Call(false, nullptr);
				return;
			}
			inCallback.Call(true, dirPath);
		}
		
	} // namespace file
} // namespace hermit
