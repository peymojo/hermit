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

#include <CoreFoundation/CoreFoundation.h>
#include <CoreServices/CoreServices.h> // for error codes fnfErr, nsvErr
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "CreateFilePathFromComponents.h"
#include "FilePathToCocoaPathString.h"
#include "GetAliasTarget.h"

namespace hermit {
	namespace file {
		namespace GetAliasTarget_Mac_Impl {

			//
			std::string CFStringToStdString(CFStringRef inStringRef) {
				const int kBufferSize = 30768;
				std::vector<char> buf(kBufferSize);
				if (!CFStringGetCString(inStringRef, &buf.at(0), kBufferSize, kCFStringEncodingUTF8)) {
					return std::string();
				}
				return std::string(&buf.at(0));
			}
			
			//
			typedef std::vector<std::string> StringVector;
			
			//
			std::string SwapSeparators(const std::string& inPath) {
				if (inPath == "/") {
					return inPath;
				}
				std::string result;
				for (std::string::size_type n = 0; n < inPath.size(); ++n) {
					std::string::value_type ch = inPath[n];
					if (ch == '/') {
						ch = ':';
					}
					else if (ch == ':') {
						ch = '/';
					}
					result.push_back(ch);
				}
				return result;
			}
			
			//
			void SplitPath(const std::string& inPath, StringVector& outNodes) {
				StringVector nodes;
				std::string path(inPath);
				while (true) {
					if (path.empty()) {
						break;
					}
					std::string::size_type sepPos = path.find("/");
					if (sepPos == std::string::npos) {
						nodes.push_back(SwapSeparators(path));
						break;
					}
					if (sepPos == 0) {
						if (nodes.empty()) {
							nodes.push_back(std::string("/"));
						}
					}
					else {
						nodes.push_back(SwapSeparators(path.substr(0, sepPos)));
					}
					path = path.substr(sepPos + 1);
				}
				outNodes.swap(nodes);
			}
			
		} // namespace GetAliasTarget_Mac_Impl
		using namespace GetAliasTarget_Mac_Impl;
		
		//
		void GetAliasTarget(const HermitPtr& h_,
							const FilePathPtr& inFilePath,
							const GetAliasTargetCallbackRef& inCallback) {
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			
			CFURLRef fileURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
																	   (const UInt8*)pathUTF8.data(),
																	   pathUTF8.size(),
																	   false);
			
			if (fileURL == nil) {
				NOTIFY_ERROR(h_, "GetAliasTarget(): CFURLCreateFromFileSystemRepresentation failed for:", inFilePath);
				inCallback.Call(kGetAliasTargetStatus_Error, 0, false);
				return;
			}
			
			CFErrorRef errorRef = nil;
			CFDataRef bookmarkData = CFURLCreateBookmarkDataFromFile(kCFAllocatorDefault, fileURL, &errorRef);
			CFRelease(fileURL);
			fileURL = nil;
			
			if (errorRef != nil) {
				if (bookmarkData != nil) {
					CFRelease(bookmarkData);
					bookmarkData = nil;
				}
				CFIndex errorCode = CFErrorGetCode(errorRef);
				CFStringRef errorDomain = CFErrorGetDomain(errorRef);
				std::string errorDomainStr(CFStringToStdString(errorDomain));

				// 256 = NSFileReadUnknownError (can't seem to include FoundationErrors.h in this file)
				if ((errorDomainStr == "NSCocoaErrorDomain") && (errorCode == 256)) {
					//	This is dubious, but in the Filehaven-Test-DeadAliases example this is where we encounter an error,
					//	at least under OS X 10 Yosemite.
					inCallback.Call(kGetAliasTargetStatus_TargetNotFound, 0, false);
					return;
				}
				
				std::string description;
				CFStringRef errorDescription = CFErrorCopyDescription(errorRef);
				if (errorDescription != nil) {
					description = CFStringToStdString(errorDescription);
					CFRelease(errorDescription);
					errorDescription = nil;
				}
				std::string failureReason;
				CFStringRef errorFailureReason = CFErrorCopyFailureReason(errorRef);
				if (errorFailureReason != nil) {
					failureReason = CFStringToStdString(errorFailureReason);
					CFRelease(errorFailureReason);
					errorFailureReason = nil;
				}
				
				NOTIFY_ERROR(h_,
							 "GetAliasTarget(): CFURLCreateBookmarkDataFromFile failed for path:", inFilePath,
							 "errorDomain:", CFStringToStdString(errorDomain),
							 "errorCode:", (int32_t)errorCode,
							 "description:", description,
							 "reason:", failureReason);
				inCallback.Call(kGetAliasTargetStatus_Error, 0, false);
				return;
			}
			
			//	try getting the target path directly from the bookmark data without resolving. if that fails,
			//	try to resolve.
			std::string aliasPath;
			CFStringRef pathEntry = (CFStringRef)CFURLCreateResourcePropertyForKeyFromBookmarkData(kCFAllocatorDefault, kCFURLPathKey, bookmarkData);
			if (pathEntry != nil) {
				CFRelease(bookmarkData);
				bookmarkData = nil;
				
				aliasPath = CFStringToStdString(pathEntry);
				CFRelease(pathEntry);
				pathEntry = nil;
			}
			else {
				Boolean isStale = false;
				CFURLRef targetURL = CFURLCreateByResolvingBookmarkData(kCFAllocatorDefault,
																		bookmarkData,
																		kCFBookmarkResolutionWithoutUIMask | kCFBookmarkResolutionWithoutMountingMask,
																		nil,
																		nil,
																		&isStale,
																		&errorRef);
				
				CFRelease(bookmarkData);
				bookmarkData = nil;
				
				if (errorRef != 0) {
					if (targetURL != nil) {
						CFRelease(targetURL);
						targetURL = nil;
					}
					
					CFStringRef errorDomain = CFErrorGetDomain(errorRef);
					CFIndex errorCode = CFErrorGetCode(errorRef);
					
					if (((CFStringCompare(errorDomain, CFSTR("NSCocoaErrorDomain"), 0) == kCFCompareEqualTo) && (errorCode == 4)) ||
						((CFStringCompare(errorDomain, CFSTR("NSOSStatusErrorDomain"), 0) == kCFCompareEqualTo) && (errorCode == fnfErr)) ||
						((CFStringCompare(errorDomain, CFSTR("NSOSStatusErrorDomain"), 0) == kCFCompareEqualTo) && (errorCode == nsvErr))) {
						inCallback.Call(kGetAliasTargetStatus_TargetNotFound, 0, false);
						return;
					}
					
					std::string description;
					CFStringRef errorDescription = CFErrorCopyDescription(errorRef);
					if (errorDescription != nil) {
						description = CFStringToStdString(errorDescription);
						CFRelease(errorDescription);
						errorDescription = nil;
					}
					std::string failureReason;
					CFStringRef errorFailureReason = CFErrorCopyFailureReason(errorRef);
					if (errorFailureReason != nil) {
						failureReason = CFStringToStdString(errorFailureReason);
						CFRelease(errorFailureReason);
						errorFailureReason = nil;
					}
					NOTIFY_ERROR(h_,
								 "GetAliasTarget(): CFURLCreateByResolvingBookmarkData failed for path:", inFilePath,
								 "errorDomain:", CFStringToStdString(errorDomain),
								 "errorCode:", (int32_t)errorCode,
								 "description:", description,
								 "reason:", failureReason);					
					inCallback.Call(kGetAliasTargetStatus_Error, 0, false);
					return;
				}
				
				const UInt32 kMaxSize = 32768;
				std::vector<UInt8> path(kMaxSize);
				Boolean success = CFURLGetFileSystemRepresentation(targetURL, false, &path.at(0), kMaxSize);
				CFRelease(targetURL);
				targetURL = nil;
				
				if (!success) {
					NOTIFY_ERROR(h_, "GetAliasTarget(): CFURLGetFileSystemRepresentation failed for path:", inFilePath);
					inCallback.Call(kGetAliasTargetStatus_Error, 0, false);
					return;
				}
				aliasPath = std::string((const char*)&path.at(0), path.size());
			}
			if (aliasPath.empty()) {
				inCallback.Call(kGetAliasTargetStatus_TargetNotFound, 0, false);
				return;
			}
			
			StringVector nodes;
			SplitPath(aliasPath, nodes);
			
			FilePathPtr destinationPath;
			CreateFilePathFromComponents(nodes, destinationPath);
			if (destinationPath == 0) {
				NOTIFY_ERROR(h_, "GetAliasTarget(): CreateFilePathFromComponents failed for target path:", aliasPath);
				inCallback.Call(kGetAliasTargetStatus_Error, 0, false);
				return;
			}
			
			inCallback.Call(kGetAliasTargetStatus_Success, destinationPath, (aliasPath[0] != '/'));
		}
		
	} // namespace file
} // namespace hermit
