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
#import "PathIsDirectory.h"
#import "CreateDirectory.h"

namespace hermit {
	namespace file {
		
		//
		CreateDirectoryResult CreateDirectory(const HermitPtr& h_, const FilePathPtr& path) {
			@autoreleasepool {
				if (path == nullptr) {
					NOTIFY_ERROR(h_, "CreateDirectory: path is null.");
					return CreateDirectoryResult::kError;
				}
				
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, path, pathUTF8);
				if (pathUTF8.empty()) {
					NOTIFY_ERROR(h_, "CreateDirectory: FilePathToCocoaPathString failed for path:", path);
					return CreateDirectoryResult::kError;
				}
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSError* error = nil;
				[[NSFileManager defaultManager] createDirectoryAtPath:pathString withIntermediateDirectories:NO attributes:nil error:&error];
				if (error != nil) {
					if ([error.domain isEqualToString:NSCocoaErrorDomain] && (error.code == NSFileWriteFileExistsError)) {
						bool isDirectory = false;
						auto status = PathIsDirectory(h_, path, isDirectory);
						if (status != PathIsDirectoryStatus::kSuccess) {
							NOTIFY_ERROR(h_, "CreateDirectory: PathIsDirectory failed for path:", path);
							return CreateDirectoryResult::kError;
						}
						if (!isDirectory) {
							return CreateDirectoryResult::kConflictAtPath;
						}
						// we didn't create it, but it's already there and a directory, so consider that a success
						return CreateDirectoryResult::kSuccess;
					}
					if ([error.domain isEqualToString:NSCocoaErrorDomain] && (error.code == NSFileWriteOutOfSpaceError)) {
						return CreateDirectoryResult::kDiskFull;
					}

					NOTIFY_ERROR(h_,
								 "CreateDirectory: createDirectoryAtPath failed for path:", path,
								 "error.localizedDescription:", [error.localizedDescription UTF8String],
								 "error.doman:", [error.domain UTF8String],
								 "error.code:", (int32_t)error.code);
					return CreateDirectoryResult::kError;
				}
			}
			return CreateDirectoryResult::kSuccess;
		}
		
	} // namespace file
} // namespace hermit
