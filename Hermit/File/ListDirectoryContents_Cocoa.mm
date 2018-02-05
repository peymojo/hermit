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
#import "Hermit/String/AddTrailingSlash.h"
#import "AppendToFilePath.h"
#import "FilePathToCocoaPathString.h"
#import "ListDirectoryContents.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			static ListDirectoryContentsResult ProcessDirectory(const HermitPtr& h_,
																const FilePathPtr& directoryPath,
																const bool& descendSubdirectories,
																ListDirectoryContentsItemCallback& itemCallback) {
				
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, directoryPath, pathUTF8);
				string::AddTrailingSlash(pathUTF8, pathUTF8);
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSError* error = nullptr;
				NSArray* items = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pathString error:&error];
				if (error != nullptr) {
					NSInteger errorCode = [error code];
					if (errorCode == NSFileReadNoSuchFileError) {
						return ListDirectoryContentsResult::kDirectoryNotFound;
					}
					if (errorCode == NSFileReadNoPermissionError) {
						return ListDirectoryContentsResult::kPermissionDenied;
					}
					
					NOTIFY_ERROR(h_,
								 "ListDirectoryContents: contentsOfDirectoryAtPath failed for path:", directoryPath,
								 "error:", [[error localizedDescription] UTF8String],
								 "error code:", (int32_t)errorCode);
					
					return ListDirectoryContentsResult::kError;
				}
				
				NSInteger numItems = [items count];
				for (NSInteger n = 0; n < numItems; ++n) {
					NSString* fileName = [items objectAtIndex:n];
					std::string fileNameUTF8([fileName cStringUsingEncoding:NSUTF8StringEncoding]);
					for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n) {
						if (fileNameUTF8[n] == ':') {
							fileNameUTF8[n] = '/';
						}
					}
					if (!itemCallback.OnItem(h_, ListDirectoryContentsResult::kSuccess, directoryPath, fileNameUTF8)) {
						return ListDirectoryContentsResult::kCanceled;
					}
				}
				
				if (descendSubdirectories) {
					for (int n = 0; n < numItems; ++n) {
						NSString* fileName = [items objectAtIndex:n];
						
						std::string itemPathUTF8(pathUTF8);
						itemPathUTF8 += [fileName cStringUsingEncoding:NSUTF8StringEncoding];
						
						struct stat s;
						int result = lstat(itemPathUTF8.c_str(), &s);
						if (result != 0) {
							NOTIFY_ERROR(h_,
										 "ListDirectoryContents: lstat failed for path:", itemPathUTF8,
										 "err:", errno);
							return ListDirectoryContentsResult::kError;
						}
						BOOL isSymbolicLink = S_ISLNK(s.st_mode);
						BOOL isDirectory = S_ISDIR(s.st_mode);
						if (!isSymbolicLink && isDirectory) {
							std::string fileNameUTF8([fileName cStringUsingEncoding:NSUTF8StringEncoding]);
							for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n) {
								if (fileNameUTF8[n] == ':') {
									fileNameUTF8[n] = '/';
								}
							}
							
							FilePathPtr filePath;
							AppendToFilePath(h_, directoryPath, fileNameUTF8, filePath);
							if (filePath == nullptr) {
								NOTIFY_ERROR(h_,
											 "ListDirectoryContents: AppendToFilePath failed for path:", directoryPath,
											 "leaf name:", fileNameUTF8);
								return ListDirectoryContentsResult::kError;
							}
							
							auto result = ProcessDirectory(h_, filePath, descendSubdirectories, itemCallback);
							if (result != ListDirectoryContentsResult::kSuccess) {
								return result;
							}
						}
					}
				}
				return ListDirectoryContentsResult::kSuccess;
			}
			
		} // private namespace
		
		//
		ListDirectoryContentsResult ListDirectoryContents(const HermitPtr& h_,
														  const FilePathPtr& directoryPath,
														  const bool& descendSubdirectories,
														  ListDirectoryContentsItemCallback& itemCallback) {
			
			@autoreleasepool {
				return ProcessDirectory(h_, directoryPath, descendSubdirectories, itemCallback);
			}
		}
		
	} // namespace file
} // namespace hermit
