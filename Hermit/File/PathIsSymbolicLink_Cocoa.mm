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
#import "PathIsSymbolicLink.h"

namespace hermit {
	namespace file {
		
		//
		//
		void PathIsSymbolicLink(const HermitPtr& h_,
								const FilePathPtr& inFilePath,
								const PathIsSymbolicLinkCallbackRef& inCallback) {
			@autoreleasepool {
				if (inFilePath == nullptr) {
					NOTIFY_ERROR(h_, "PathIsSymbolicLink: inFilePath is null.");
					inCallback.Call(false, false);
					return;
				}
				
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				if (pathUTF8.empty()) {
					NOTIFY_ERROR(h_, "PathIsSymbolicLink: FilePathToCocoaPathString failed for path:", inFilePath);
					inCallback.Call(false, false);
					return;
				}
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				if (pathString == nil) {
					NOTIFY_ERROR(h_, "PathIsSymbolicLink: stringWithUTF8String failed for path string:", pathUTF8);
					NOTIFY_ERROR(h_, "-- path:", inFilePath);
					inCallback.Call(false, false);
					return;
				}
				
				NSURL* fileURL = [NSURL fileURLWithPath:pathString];
				if (fileURL == nil) {
					NOTIFY_ERROR(h_, "PathIsSymbolicLink: initFileURLWithPath failed for path string:", [pathString UTF8String]);
					NOTIFY_ERROR(h_, "-- path:", inFilePath);
					inCallback.Call(false, false);
					return;
				}
				
				NSArray* keys = [NSArray arrayWithObject:NSURLIsSymbolicLinkKey];
				NSError* error = nil;
				NSDictionary* dict = [fileURL resourceValuesForKeys:keys error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_, "PathIsSymbolicLink: resourceValuesForKeys failed for path:", inFilePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "-- error.description:", [error.description UTF8String]);
						NOTIFY_ERROR(h_, "-- error.domain:", [error.domain UTF8String]);
						NOTIFY_ERROR(h_, "-- error.code:", (int32_t)error.code);
					}
					inCallback.Call(false, false);
					return;
				}
				
				NSNumber* isLink = [dict objectForKey:NSURLIsSymbolicLinkKey];
				inCallback.Call(true, [isLink boolValue]);
			}
		}
		
	} // namespace file
} // namespace hermit
