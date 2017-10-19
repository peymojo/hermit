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

#import <string>
#import <sys/stat.h>
#import "FilePathToCocoaPathString.h"
#import "GetFileDates.h"

namespace hermit {
	namespace file {
		
		//
		void GetFileDates(const HermitPtr& h_,
						  const file::FilePathPtr& inFilePath,
						  const file::GetFileDatesCallbackRef& inCallback) {
			
			@autoreleasepool {
				std::string pathUTF8;
				file::FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				
				// NOTE: at one point i tried using NSFileManager here to see if it was any faster
				// but it didn't appear to be... could take another look at some point
				struct stat fileInfo;
				lstat(pathUTF8.c_str(), &fileInfo);
				
				struct tm created;
				gmtime_r(&fileInfo.st_birthtimespec.tv_sec, &created);
				
				char createdString[256];
				size_t createdUsed = strftime(createdString, 256, "%Y-%m-%d %H:%M:%S UTC", &created);
				if (createdUsed == 0) {
					inCallback.Call(false, "", "");
					return;
				}
				
				struct tm modified;
				gmtime_r(&fileInfo.st_mtimespec.tv_sec, &modified);
				
				char modifiedString[256];
				size_t modifiedUsed = strftime(modifiedString, 256, "%Y-%m-%d %H:%M:%S UTC", &modified);
				if (modifiedUsed == 0) {
					inCallback.Call(false, "", "");
					return;
				}
				
				inCallback.Call(true, createdString, modifiedString);
			}
		}
		
	} // namespace file
} // namespace hermit
