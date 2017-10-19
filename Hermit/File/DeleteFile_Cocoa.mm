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

#import <Foundation/Foundation.h>
#import <string>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "DeleteFile.h"

namespace hermit {
	namespace file {
		
		//
		DeleteFileStatus DeleteFile(const HermitPtr& h_, const FilePathPtr& inFilePath) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSError* error = nil;
				BOOL success = [[NSFileManager defaultManager] removeItemAtPath:pathString error:&error];
				if (success == NO) {
					NSInteger code = [error code];
					if (code == NSFileNoSuchFileError) {
						return kDeleteFileStatus_FileNotFound;
					}

					NOTIFY_ERROR(h_, "DeleteFile: removeItemAtPath failed for path:", inFilePath);
					NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
					NOTIFY_ERROR(h_, "-- code:", (int)code);
					
					return kDeleteFileStatus_Error;
				}
			}
			return kDeleteFileStatus_Success;
		}
		
	} // namespace file
} // namespace hermit
