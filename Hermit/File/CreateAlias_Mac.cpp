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
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "WriteFileData.h"
#include "CreateAlias.h"

namespace hermit {
	namespace file {
		
		//
		bool CreateAlias(const HermitPtr& h_, const FilePathPtr& filePath, const FilePathPtr& targetFilePath) {
			std::string filePathUTF8;
			FilePathToCocoaPathString(h_, filePath, filePathUTF8);
			
			std::string targetPathUTF8;
			FilePathToCocoaPathString(h_, targetFilePath, targetPathUTF8);
			
			CFURLRef targetURL = CFURLCreateFromFileSystemRepresentation(kCFAllocatorDefault,
																		 (const UInt8*)targetPathUTF8.c_str(),
																		 targetPathUTF8.size(),
																		 FALSE);
			
			//	CFURLRef relativeToURL = CFURLCreateFromFileSystemRepresentation(
			//		kCFAllocatorDefault,
			//		(const UInt8*)filePathUTF8.c_str(),
			//		filePathUTF8.size(),
			//		FALSE);
			
			CFErrorRef error = 0;
			CFDataRef bookmarkData = CFURLCreateBookmarkData(kCFAllocatorDefault,
															 targetURL,
															 kCFURLBookmarkCreationSuitableForBookmarkFile,
															 nullptr,
															 nullptr,
															 &error);
			
			//	CFRelease(relativeToURL);
			CFRelease(targetURL);
			
			if (bookmarkData == 0) {
				NOTIFY_ERROR(h_,
							 "CreateAlias: CFURLCreateBookmarkDataFromAliasRecord failed for alias path:",
							 filePath);
				NOTIFY_ERROR(h_, "-- target path:", targetFilePath);
				
				if (error != 0) {
					CFIndex code = CFErrorGetCode(error);
					CFStringRef description = CFErrorCopyDescription(error);
					
					CFIndex stringLength = CFStringGetLength(description);
					if (stringLength > 0) {
						size_t bufferSize = stringLength * 4;
						std::vector<char> descriptionStr(bufferSize);
						Boolean stringResult = CFStringGetCString(description,
																  &descriptionStr.at(0),
																  bufferSize,
																  kCFStringEncodingUTF8);
						
						if (stringResult == TRUE) {
							NOTIFY_ERROR(h_, "-- description:", &descriptionStr.at(0));
						}
					}
					NOTIFY_ERROR(h_, "-- code:", (int32_t)code);
					
					CFRelease(description);
					CFRelease(error);
				}
				return false;
			}
			
			CFStringRef aliasPathRef = CFStringCreateWithBytes(kCFAllocatorDefault,
															   (const UInt8*)filePathUTF8.c_str(),
															   filePathUTF8.size(),
															   kCFStringEncodingUTF8,
															   false);
			
			CFURLRef aliasFileURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault,
																  aliasPathRef,
																  kCFURLPOSIXPathStyle,
																  false);
			
			CFRelease(aliasPathRef);
			
			//	CFURLCreateFromFileSystemRepresentation(
			//		kCFAllocatorDefault,
			//		(const UInt8*)filePathUTF8.c_str(),
			//		filePathUTF8.size(),
			//		FALSE);
			
			if (aliasFileURL == 0) {
				CFRelease(bookmarkData);
				
				NOTIFY_ERROR(h_,
							 "CreateAlias: CFURLCreateFromFileSystemRepresentation failed for alias path:",
							 filePath);
				NOTIFY_ERROR(h_, "-- target path:", targetFilePath);

				return false;
			}
			
			error = 0;
			Boolean writeResult = CFURLWriteBookmarkDataToFile(bookmarkData,
															   aliasFileURL,
															   0,
															   &error);
			
			CFRelease(aliasFileURL);
			CFRelease(bookmarkData);
			
			if (writeResult == FALSE) {
				NOTIFY_ERROR(h_,
							 "CreateAlias: CFURLWriteBookmarkDataToFile failed for alias path:",
							 filePath);
				NOTIFY_ERROR(h_, "-- target path:", targetFilePath);
				
				if (error != 0) {
					CFIndex code = CFErrorGetCode(error);
					CFStringRef description = CFErrorCopyDescription(error);
					
					CFIndex stringLength = CFStringGetLength(description);
					if (stringLength > 0) {
						size_t bufferSize = stringLength * 4;
						std::vector<char> descriptionStr(bufferSize);
						Boolean stringResult = CFStringGetCString(description,
																  &descriptionStr.at(0),
																  bufferSize,
																  kCFStringEncodingUTF8);
						
						if (stringResult == TRUE) {
							NOTIFY_ERROR(h_, "-- description:", &descriptionStr.at(0));
						}
					}
					
					NOTIFY_ERROR(h_, "-- code:", (int32_t)code);
					CFRelease(description);
					CFRelease(error);
				}
				
				return false;
			}
			
			return true;
		}
		
	} // namespace file
} // namespace hermit
