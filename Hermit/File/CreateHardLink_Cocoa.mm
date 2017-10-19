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
#import "CreateHardLink.h"

namespace hermit {
	namespace file {
		
		//
		bool CreateHardLink(const HermitPtr& h_, const FilePathPtr& hardLinkFilePath, const FilePathPtr& originalItemFilePath) {
			@autoreleasepool {
				std::string hardLinkPath;
				FilePathToCocoaPathString(h_, hardLinkFilePath, hardLinkPath);
				if (hardLinkPath.empty()) {
					NOTIFY_ERROR(h_,
								 "CreateHardLink: FilePathToCocoaPathString failed for hard link path:",
								 hardLinkFilePath);
					return false;
				}
				
				std::string originalItemPath;
				FilePathToCocoaPathString(h_, originalItemFilePath, originalItemPath);
				if (originalItemPath.empty()) {
					NOTIFY_ERROR(h_,
								 "CreateHardLink: FilePathToCocoaPathString failed for original item path:",
								 originalItemFilePath);
					return false;
				}
				
				NSString* hardLinkPathStr = [NSString stringWithUTF8String:hardLinkPath.c_str()];
				NSString* originalItemPathStr = [NSString stringWithUTF8String:originalItemPath.c_str()];
				
				NSError* error = nil;
				if (![[NSFileManager defaultManager] linkItemAtPath:originalItemPathStr toPath:hardLinkPathStr error:&error]) {
					NOTIFY_ERROR(h_, "CreateHardLink: linkItemAtPath failed for hard link path:", hardLinkFilePath);
					NOTIFY_ERROR(h_, "-- original item path:", originalItemFilePath);
					NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
					NOTIFY_ERROR(h_, "-- code:", (int32_t)[error code]);
					
					return false;
				}
				return true;
			}
		}
		
	} // namespace file
} // namespace hermit
