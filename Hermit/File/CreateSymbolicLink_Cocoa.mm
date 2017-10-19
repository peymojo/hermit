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
#import "CreateSymbolicLink.h"

namespace hermit {
	namespace file {
		
		//
		//
		bool CreateSymbolicLink(const HermitPtr& h_, const FilePathPtr& linkPath, const FilePathPtr& targetPath) {
			@autoreleasepool {
				std::string linkPathUTF8;
				FilePathToCocoaPathString(h_, linkPath, linkPathUTF8);
				std::string targetPathUTF8;
				FilePathToCocoaPathString(h_, targetPath, targetPathUTF8);
				
				NSString* linkPath = [NSString stringWithUTF8String:linkPathUTF8.c_str()];
				NSString* targetPath = [NSString stringWithUTF8String:targetPathUTF8.c_str()];
				NSError* error = nil;
				BOOL result = [[NSFileManager defaultManager] createSymbolicLinkAtPath:linkPath withDestinationPath:targetPath error:&error];
				if (error != nil) {
					NOTIFY_ERROR(h_,
								 "CreateSymbolicLink(): NSFileManager createSymbolicLinkAtPath failed for path:",
								 linkPath);
					
					NSInteger errorCode = [error code];
					NOTIFY_ERROR(h_, "\terror code:", (int32_t)errorCode);
					NOTIFY_ERROR(h_, "\terror:", [[error localizedDescription] UTF8String]);
					
					result = FALSE;
				}
				else if (!result) {
					NOTIFY_ERROR(h_,
								 "CreateSymbolicLink(): NSFileManager createSymbolicLinkAtPath failed for path:",
								 linkPath);
				}
				
				return (result == TRUE);
			}
		}
		
	} // namespace file
} // namespace hermit
