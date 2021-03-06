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
#import "GetFilePosixOwnership.h"

namespace hermit {
	namespace file {
		
		//
		bool GetFilePosixOwnership(const HermitPtr& h_, const file::FilePathPtr& inFilePath, std::string& outUserOwner, std::string& outGroupOwner) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSError* error = nil;
				NSDictionary* dict = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_, "GetFilePosixOwnership: attributesOfItemAtPath failed for path:", inFilePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
					}
					return false;
				}
				NSString* userOwner = [dict fileOwnerAccountName];
				std::string userOwnerStr;
				if (userOwner != nil) {
					userOwnerStr = [userOwner UTF8String];
				}
				NSString* groupOwner = [dict fileGroupOwnerAccountName];
				std::string groupOwnerStr;
				if (groupOwner != nil) {
					groupOwnerStr = [groupOwner UTF8String];
				}
				outUserOwner = userOwnerStr;
				outGroupOwner = groupOwnerStr;
				return true;
			}
		}
		
	} // namespace file
} // namespace hermit
