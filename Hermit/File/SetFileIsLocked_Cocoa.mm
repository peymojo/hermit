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
#import "SetFileIsLocked.h"

namespace hermit {
	namespace file {
		
		//
		bool SetFileIsLocked(const HermitPtr& h_, const FilePathPtr& filePath, bool isLocked) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, filePath, pathUTF8);
				
				//		PathIsSymbolicLinkCallbackClass isLinkCallback;
				//		PathIsSymbolicLink(inHub, inFilePath, isLinkCallback);
				//		if (!isLinkCallback.mSuccess)
				//		{
				//			LogFilePath(
				//				inHub,
				//				"SetFilePosixOwnership: PathIsSymbolicLink failed for path: ",
				//				inFilePath);
				//
				//			inCallback.Call(inHub, false);
				//			return;
				//		}
				//		if (isLinkCallback.mIsLink)
				//		{
				//			int userId = 0;
				//			if (!GetUserID(inHub, inUserOwner, userId))
				//			{
				//				NOTIFY_ERROR(
				//					inHub,
				//					"SetFilePosixOwnership: GetUserID failed for user:",
				//					inUserOwner);
				//
				//				inCallback.Call(inHub, false);
				//				return;
				//			}
				//
				//			int groupId = 0;
				//			if (!GetGroupID(inHub, inGroupOwner, groupId))
				//			{
				//				NOTIFY_ERROR(
				//					inHub,
				//					"SetFilePosixOwnership: GetGroupID failed for group:",
				//					inGroupOwner);
				//
				//				inCallback.Call(inHub, false);
				//				return;
				//			}
				//
				//			int result = lchown(pathUTF8.c_str(), userId, groupId);
				//			if (result != 0)
				//			{
				//				LogSInt32(
				//					inHub,
				//					"SetFilePosixOwnership: lchown failed, result: ",
				//					result);
				//
				//				inCallback.Call(inHub, false);
				//				return;
				//			}
				//			inCallback.Call(inHub, true);
				//			return;
				//		}
				
				NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
									  [NSNumber numberWithBool:(isLocked ? YES : NO)], NSFileImmutable,
									  nil];
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSError* error = nil;
				BOOL success = [[NSFileManager defaultManager] setAttributes:dict ofItemAtPath:pathString error:&error];
				if (!success) {
					NOTIFY_ERROR(h_, "SetFileIsLocked: setAttributes:ofItemAtPath: failed for path:", filePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "SetFileIsLocked: error:", [[error localizedDescription] UTF8String]);
					}
				}
				return success;
			}
		}
		
	} // namespace file
} // namespace hermit
