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
#import <sys/stat.h>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "PathIsSymbolicLink.h"
#import "SetFilePosixPermissions.h"

namespace hermit {
	namespace file {
		
		//
		bool SetFilePosixPermissions(const HermitPtr& h_,
									 const FilePathPtr& inFilePath,
									 const uint32_t& inPosixPermissions) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				
				PathIsSymbolicLinkCallbackClass isLinkCallback;
				PathIsSymbolicLink(h_, inFilePath, isLinkCallback);
				if (!isLinkCallback.mSuccess) {
					NOTIFY_ERROR(h_, "SetFilePosixPermissions: PathIsSymbolicLink failed for path:", inFilePath);
					return false;
				}
				if (isLinkCallback.mIsLink) {
					int result = lchmod(pathUTF8.c_str(), inPosixPermissions);
					if (result != 0) {
						NOTIFY_ERROR(h_, "SetFilePosixPermissions: lchmod failed, result:", result);
						return false;
					}
					return true;
				}
				
				NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:[NSNumber numberWithUnsignedInt:inPosixPermissions], NSFilePosixPermissions, nil];
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSError* error = nil;
				BOOL success = [[NSFileManager defaultManager] setAttributes:dict ofItemAtPath:pathString error:&error];
				
				if (!success) {
					NOTIFY_ERROR(h_, "SetFilePosixPermissions: setAttributes:ofItemAtPath: failed for path:", inFilePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
					}
					return false;
				}
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit
