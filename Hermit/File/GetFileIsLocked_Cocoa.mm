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
#import "GetFileIsLocked.h"

namespace hermit {
	namespace file {
		
		//
		//
		void GetFileIsLocked(const HermitPtr& h_,
							 const FilePathPtr& inFilePath,
							 const GetFileIsLockedCallbackRef& inCallback) {
			@autoreleasepool {
				std::string cocoaPath;
				FilePathToCocoaPathString(h_, inFilePath, cocoaPath);
				
				NSString* pathString = [NSString stringWithUTF8String:cocoaPath.c_str()];
				
				NSError* error = nil;
				NSDictionary* dict = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_,
								 "GetFileIsLocked: attributesOfItemAtPath failed for path:",
								 inFilePath);
					
					if (error != nil) {
						NOTIFY_ERROR(h_,
									 "\terror:",
									 [[error localizedDescription] UTF8String]);
					}
					inCallback.Call(false, false);
				}
				else {
					BOOL isImmutable = [dict fileIsImmutable];
					inCallback.Call(true, (isImmutable == YES));
				}
			}
		}
		
	} // namespace file
} // namespace hermit
