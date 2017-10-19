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
#import <grp.h>
#import <pwd.h>
#import <string>
#import <sys/types.h>
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "GetFilePathUTF8String.h"
#import "PathIsSymbolicLink.h"
#import "SetFilePosixOwnership.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			bool GetUserID(const HermitPtr& h_, const std::string& inName, int& outUID) {
				size_t bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
				if (bufsize == -1) {
					bufsize = 16384;
				}
				char* buf = (char*)malloc(bufsize);
				if (buf == nullptr) {
					NOTIFY_ERROR(h_, "GetUserID: malloc failed.");
					return false;
				}
				
				struct passwd passwdInfo = { 0 };
				struct passwd* pwResult = nullptr;
				int result = getpwnam_r(inName.c_str(), &passwdInfo, buf, bufsize, &pwResult);
				if (pwResult == nullptr) {
					NOTIFY_ERROR(h_, "GetUserID: getpwnam_r failed, result:", result);
					free(buf);
					return false;
				}
				outUID = pwResult->pw_uid;
				free(buf);
				return true;
			}
			
			//
			//
			bool GetGroupID(const HermitPtr& h_, const std::string& inName, int& outUID) {
				size_t bufsize = sysconf(_SC_GETGR_R_SIZE_MAX);
				if (bufsize == -1) {
					bufsize = 16384;
				}
				char* buf = (char*)malloc(bufsize);
				if (buf == nullptr) {
					NOTIFY_ERROR(h_, "GetGroupID: malloc failed.");
					return false;
				}
				
				struct group groupInfo = { 0 };
				struct group* grpResult = nullptr;
				int result = getgrnam_r(inName.c_str(), &groupInfo, buf, bufsize, &grpResult);
				if (grpResult == nullptr) {
					NOTIFY_ERROR(h_, "GetGroupID: getpwnam_r failed, result:", result);
					free(buf);
					return false;
				}
				outUID = grpResult->gr_gid;
				free(buf);
				return true;
			}
			
		} // private namespace
		
		//
		SetFilePosixOwnershipResult SetFilePosixOwnership(const HermitPtr& h_,
														  const FilePathPtr& filePath,
														  const std::string& userOwner,
														  const std::string& groupOwner) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, filePath, pathUTF8);
				
				PathIsSymbolicLinkCallbackClass isLinkCallback;
				PathIsSymbolicLink(h_, filePath, isLinkCallback);
				if (!isLinkCallback.mSuccess) {
					NOTIFY_ERROR(h_, "SetFilePosixOwnership: PathIsSymbolicLink failed for path:", filePath);
					return SetFilePosixOwnershipResult::kError;
				}
				if (isLinkCallback.mIsLink) {
					int userId = 0;
					if (!GetUserID(h_, userOwner, userId)) {
						NOTIFY_ERROR(h_, "SetFilePosixOwnership: GetUserID failed for user:", userOwner);
						return SetFilePosixOwnershipResult::kError;
					}
					
					int groupId = 0;
					if (!GetGroupID(h_, groupOwner, groupId)) {
						NOTIFY_ERROR(h_, "SetFilePosixOwnership: GetGroupID failed for group:", groupOwner);
						return SetFilePosixOwnershipResult::kError;
					}
					
					int result = lchown(pathUTF8.c_str(), userId, groupId);
					if (result != 0) {
						int err = errno;
						if (err == EPERM) {
							return SetFilePosixOwnershipResult::kPermissionDenied;
						}
						NOTIFY_ERROR(h_, "SetFilePosixOwnership: lchown failed, err:", err);
						return SetFilePosixOwnershipResult::kError;
					}
					return SetFilePosixOwnershipResult::kSuccess;
				}
				
				NSDictionary* dict = [NSDictionary dictionaryWithObjectsAndKeys:
									  [NSString stringWithUTF8String:userOwner.c_str()], NSFileOwnerAccountName,
									  [NSString stringWithUTF8String:groupOwner.c_str()], NSFileGroupOwnerAccountName,
									  nil];
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSError* error = nil;
				BOOL success = [[NSFileManager defaultManager] setAttributes:dict ofItemAtPath:pathString error:&error];
				if (!success) {
					if (error == nil) {
						NOTIFY_ERROR(h_, "SetFilePosixOwnership: setAttributes:ofItemAtPath: failed:", filePath);
						return SetFilePosixOwnershipResult::kError;
					}
					
					NSString* domain = error.domain;
					NSInteger code = error.code;
					if ([domain isEqualToString:NSCocoaErrorDomain] && (code == NSFileWriteNoPermissionError)) {
						return SetFilePosixOwnershipResult::kPermissionDenied;
					}
					
					NOTIFY_ERROR(h_, "SetFilePosixOwnership: setAttributes:ofItemAtPath: failed for path:", filePath);
					NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
					return SetFilePosixOwnershipResult::kError;
				}
				return SetFilePosixOwnershipResult::kSuccess;
			}
		}
		
	} // namespace file
} // namespace hermit
