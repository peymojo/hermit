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
#import "PathIsHardLink.h"

namespace hermit {
	namespace file {
		
		//
		void PathIsHardLink(const HermitPtr& h_,
							const FilePathPtr& inFilePath,
							const PathIsHardLinkCallbackRef& inCallback) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSError* error = nil;
				NSDictionary* dict = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_,
								 "PathIsHardLink(): attributesOfItemAtPath failed for path:",
								 inFilePath);
					
					if (error != nil) {
						NOTIFY_ERROR(h_,
									 "\terror:",
									 [[error localizedDescription] UTF8String]);
					}
					inCallback.Call(false, false, 0, 0);
				}
				else {
					uint32_t refCount = 0;
					uint32_t fileSystemNumber = 0;
					uint32_t fileNumber = 0;
					
					NSNumber* refCountNumberObj = [dict objectForKey:NSFileReferenceCount];
					if ((refCountNumberObj != nil) && ([refCountNumberObj isKindOfClass:[NSNumber class]])) {
						refCount = (uint32_t)[refCountNumberObj integerValue];
					}
					
					NSNumber* fileSystemNumberObj = [dict objectForKey:NSFileSystemNumber];
					if ((fileSystemNumberObj != nil) && ([fileSystemNumberObj isKindOfClass:[NSNumber class]])) {
						fileSystemNumber = (uint32_t)[fileSystemNumberObj integerValue];
					}
					
					NSNumber* fileNumberObj = [dict objectForKey:NSFileSystemFileNumber];
					if ((fileNumberObj != nil) && ([fileNumberObj isKindOfClass:[NSNumber class]])) {
						fileNumber = (uint32_t)[fileNumberObj integerValue];
					}
					inCallback.Call(true, (refCount > 1), fileSystemNumber, fileNumber);
				}
			}
		}
		
	} // namespace file
} // namespace hermit
