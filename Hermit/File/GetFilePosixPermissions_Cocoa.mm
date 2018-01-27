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

#import <Cocoa/Cocoa.h>
#import <string>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "GetFilePosixPermissions.h"

namespace hermit {
	namespace file {
		
		//
		bool GetFilePosixPermissions(const HermitPtr& h_, const FilePathPtr& filePath, uint32_t& outPermissions) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, filePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSError* error = nil;
				NSDictionary* dict = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_, "GetFilePosixPermissions: attributesOfItemAtPath failed for path:", filePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "error:", [[error localizedDescription] UTF8String]);
					}
					return false;
				}

				outPermissions = (uint32_t)[dict filePosixPermissions];
				return true;
			}
		}
		
	} // namespace file
} // namespace hermit
