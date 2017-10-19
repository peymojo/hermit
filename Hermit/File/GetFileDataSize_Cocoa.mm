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
#import <Foundation/FoundationErrors.h>
#import <string>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "GetFileDataSize.h"

namespace hermit {
	namespace file {
		
		//
		bool GetFileDataSize(const HermitPtr& h_, const FilePathPtr& filePath, std::uint64_t& outDataSize) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, filePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSError* error = nil;
				NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				if (error != nil) {
					NSInteger errorCode = [error code];
					//			NSString* errorDomain = [error domain];
					if (errorCode == NSFileReadNoSuchFileError) {
						NOTIFY_ERROR(h_, "GetFileDataSize: NSFileReadNoSuchFileError for path:", filePath);
						return false;
					}
					if (errorCode == NSFileReadUnknownError) {
						// this used to be treated only as a warning... but why?
						NOTIFY_ERROR(h_, "GetFileDataSize: attributesOfItemAtPath returned NSFileReadUnknownError, path:", filePath);
						return false;
					}
					NOTIFY_ERROR(h_,
								 "GetFileDataSize: attributesOfItemAtPath failed for path:", filePath,
								 "error code:", (int32_t)errorCode,
								 "error:", [[error localizedDescription] UTF8String]);
					return false;
				}
				if (attributes == nil) {
					NOTIFY_ERROR(h_, "GetFileDataSize: attributesOfItemAtPath returned nil for path:", filePath);
					return false;
				}
				outDataSize = [attributes fileSize];
				return true;
			}
		}
		
	} // namespace file
} // namespace hermit
