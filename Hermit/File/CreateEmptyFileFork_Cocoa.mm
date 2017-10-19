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
#import "CreateEmptyFileFork.h"

namespace hermit {
	namespace file {
		
		//
		bool CreateEmptyFileFork(const HermitPtr& h_, const FilePathPtr& filePath, const std::string& forkName) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, filePath, pathUTF8);
				if (!forkName.empty()) {
					pathUTF8 += "/..namedfork/";
					if (forkName == "RESOURCE_FORK") {
						pathUTF8 += "rsrc";
					}
					else {
						pathUTF8 += forkName;
					}
				}
				
				bool success = true;
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSData* data = [NSData data];
				
				@try {
					[[NSFileManager defaultManager] createFileAtPath:pathString contents:data attributes:nil];
				}
				@catch (NSException* exception) {
					NOTIFY_ERROR(h_,
								 "CreateEmptyFileFork: createFileAtPath failed for path:", filePath,
								 "fork name:", forkName,
								 "exception.name:", [exception.name UTF8String],
								 "exception.reason:", [exception.reason UTF8String]);
					success = false;
				}
				return success;
			}
		}
		
	} // namespace file
} // namespace hermit
