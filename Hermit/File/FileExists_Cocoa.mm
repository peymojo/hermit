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

#import <errno.h>
#import <string>
#import <sys/stat.h>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "FileExists.h"

namespace hermit {
	namespace file {
		
		//
		bool FileExists(const HermitPtr& h_, const FilePathPtr& filePath, bool& outExists) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "FileExists: inFilePath is null.");
				return false;
			}
			
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, filePath, pathUTF8);
			if (pathUTF8.empty()) {
				NOTIFY_ERROR(h_, "FileExists: FilePathToCocoaPathString failed for:", filePath);
				return false;
			}
			
			struct stat fileStat = { 0 };
			int result = lstat(pathUTF8.c_str(), &fileStat);
			if (result != 0) {
				int err = errno;
				if (err == ENOENT) {
					outExists = false;
					return true;
				}
				
				NOTIFY_ERROR(h_, "FileExists: lstat failed for path:", filePath, "error:", err);
				return false;
			}
			
			outExists = true;
			return true;
		}
		
	} // namespace file
} // namespace hermit
