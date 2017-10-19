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
#import "CopySymbolicLink.h"

namespace hermit {
	namespace file {
		
		//
		bool CopySymbolicLink(const HermitPtr& h_, const FilePathPtr& sourcePath, const FilePathPtr& destPath) {
			@autoreleasepool {
				std::string sourcePathUTF8;
				FilePathToCocoaPathString(h_, sourcePath, sourcePathUTF8);
				NSString* sourcePathString = [NSString stringWithUTF8String:sourcePathUTF8.c_str()];
				
				std::string destPathUTF8;
				FilePathToCocoaPathString(h_, destPath, destPathUTF8);
				NSString* destPathString = [NSString stringWithUTF8String:destPathUTF8.c_str()];
				
				NSError* error = nil;
				BOOL result = [[NSFileManager defaultManager] copyItemAtPath:sourcePathString toPath:destPathString error:&error];
				if (error != nil) {
					NOTIFY_ERROR(h_,
								 "CopySymbolicLink(): copyItemAtPath failed for path:", sourcePath,
								 "dest path:", destPath,
								 "error code:", (int32_t)[error code],
								 "error:", [[error localizedDescription] UTF8String]);
					
					result = NO;
				}
				
				return (result == YES);
			}
		}
		
	} // namespace file
} // namespace hermit


