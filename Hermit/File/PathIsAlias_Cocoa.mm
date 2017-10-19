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
#import "PathIsAlias.h"

namespace hermit {
	namespace file {
		
		//
		void PathIsAlias(const HermitPtr& h_,
						 const FilePathPtr& inFilePath,
						 const PathIsAliasCallbackRef& inCallback) {
			
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:pathUTF8.c_str()]];
				id isAliasValue = nil;
				NSError* error = nil;
				[url getResourceValue:&isAliasValue forKey:NSURLIsAliasFileKey error:&error];
				if (error != nil) {
					NOTIFY_ERROR(h_,
								 "SetFileDates: setResourceValue:NSURLCreationDateKey failed for file:", inFilePath,
								 "error.localizedDescription:", [error.localizedDescription UTF8String],
								 "error.domain:", [error.domain UTF8String],
								 "error.code: ", (int32_t)error.code);
					inCallback.Call(false, false);
					return;
				}
				NSNumber* isAliasNumber = isAliasValue;
				inCallback.Call(true, [isAliasNumber boolValue]);
			}
		}
		
	} // namespace file
} // namespace hermit
