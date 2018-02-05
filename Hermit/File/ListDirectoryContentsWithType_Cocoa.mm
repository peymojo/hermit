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
#import <sys/stat.h>
#import <string>
#import "Hermit/Foundation/Notification.h"
#import "Hermit/String/AddTrailingSlash.h"
#import "AppendToFilePath.h"
#import "FilePathToCocoaPathString.h"
#import "ListDirectoryContentsWithType.h"

namespace hermit {
	namespace file {
		namespace ListDirectoryContentsWithType_Cocoa_Impl {
			
			//
			class Lister {
			public:
				//
				static ListDirectoryContentsResult ProcessDirectory(const HermitPtr& h_,
																	const FilePathPtr& directoryPath,
																	const bool& descendSubdirectories,
																	ListDirectoryContentsWithTypeItemCallback& itemCallback) {
					if (CHECK_FOR_ABORT(h_)) {
						return ListDirectoryContentsResult::kCanceled;
					}
					
					std::string pathUTF8;
					FilePathToCocoaPathString(h_, directoryPath, pathUTF8);
					string::AddTrailingSlash(pathUTF8, pathUTF8);					
					NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
					
					NSError* error = nil;
					NSArray* items = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pathString error:&error];
					if (error != nil) {
						auto result = ListDirectoryContentsResult::kError;
						if (error.code == NSFileReadNoPermissionError) {
							result = ListDirectoryContentsResult::kPermissionDenied;
						}
						else {
							NOTIFY_ERROR(h_,
										 "ListDirectoryContentsWithType: contentsOfDirectoryAtPath failed for path:", directoryPath,
										 "error:", [[error localizedDescription] UTF8String]);
						}
						return result;
					}
					
					NSInteger numItems = [items count];
					for (NSInteger n = 0; n < numItems; ++n) {
						NSString* fileName = [items objectAtIndex:n];
						
						std::string itemPathUTF8(pathUTF8);
						std::string fileNameUTF8([fileName cStringUsingEncoding:NSUTF8StringEncoding]);
						itemPathUTF8 += fileNameUTF8;
						
						// Using [oneURL getResourceValue...] here seems to be slower than lstat, or at least did at one
						// point. Might be worth another look to figure out why and/or see if it's still true in current
						// macOS versions.
						FileType fileType = FileType::kUnknown;
						struct stat s;
						int err = lstat(itemPathUTF8.c_str(), &s);
						if (err != 0) {
							NOTIFY_ERROR(h_,
										 "ListDirectoryContentsWithType: lstat failed for path:", itemPathUTF8,
										 "err:", err);
						}
						else if (S_ISREG(s.st_mode)) {
							fileType = FileType::kFile;
						}
						else if (S_ISDIR(s.st_mode)) {
							fileType = FileType::kDirectory;
						}
						else if (S_ISLNK(s.st_mode)) {
							fileType = FileType::kSymbolicLink;
						}

						if (fileType == FileType::kUnknown) {
							NOTIFY_ERROR(h_, "ListDirectoryContentsWithType: Unrecognized item type:", itemPathUTF8);
							if (!itemCallback.OnItem(h_,
													 ListDirectoryContentsResult::kError,
													 directoryPath,
													 fileNameUTF8,
													 fileType)) {
								return ListDirectoryContentsResult::kError;
							}
							continue;
						}
						
						for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n) {
							if (fileNameUTF8[n] == ':') {
								fileNameUTF8[n] = '/';
							}
						}
						auto result = ListDirectoryContentsResult::kSuccess;
						if (descendSubdirectories && (fileType == FileType::kDirectory)) {
							FilePathPtr filePath;
							AppendToFilePath(h_, directoryPath, fileNameUTF8, filePath);
							if (filePath == nullptr) {
								NOTIFY_ERROR(h_,
											 "ListDirectoryContentsWithType: AppendToFilePath for parent path:", directoryPath,
											 "file name:", fileNameUTF8);
								result = ListDirectoryContentsResult::kError;
							}
							else {
								result = ProcessDirectory(h_, filePath, true, itemCallback);
							}
						}
						if (!itemCallback.OnItem(h_,
												 result,
												 directoryPath,
												 fileNameUTF8,
												 fileType)) {
							return result;
						}
					}
					return ListDirectoryContentsResult::kSuccess;
				}
			};
			
		} // namespace ListDirectoryContentsWithType_Cocoa_Impl
		using namespace ListDirectoryContentsWithType_Cocoa_Impl;
		
		//
		ListDirectoryContentsResult ListDirectoryContentsWithType(const HermitPtr& h_,
																  const FilePathPtr& inDirectoryPath,
																  const bool& inDescendSubdirectories,
																  ListDirectoryContentsWithTypeItemCallback& itemCallback) {
			
			@autoreleasepool {
				return Lister::ProcessDirectory(h_, inDirectoryPath, inDescendSubdirectories, itemCallback);
			}
		}
		
	} // namespace file
} // namespace hermit
