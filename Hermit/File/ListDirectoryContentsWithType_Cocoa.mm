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
		
		namespace
		{
			
#if 000
			//	not sure why this version is slower but it seems to be:
			
			//
			//
			class Lister
			{
			public:
				//
				//
				static bool ProcessDirectory(
											 const FilePathPtr& inDirectoryPath,
											 const bool& inDescendSubdirectories,
											 const ListDirectoryContentsWithTypeCallbackRef& inCallback)
				{
					StringCallbackClass filePathUTF8;
					FilePathToCocoaPathString(inDirectoryPath, filePathUTF8);
					StringCallbackClass pathUTF8;
					string::AddTrailingSlash(filePathUTF8.mValue, pathUTF8);
					
					NSString* pathString = [NSString stringWithUTF8String:pathUTF8.mValue.c_str()];
					
					NSURL* url = [NSURL fileURLWithPath:pathString];
					NSArray* keys = @[ NSURLIsDirectoryKey, NSURLIsSymbolicLinkKey, NSURLFileSizeKey ];
					NSError* error = nil;
					NSArray* items = [[NSFileManager defaultManager]
									  contentsOfDirectoryAtURL:url
									  includingPropertiesForKeys:keys
									  options:0
									  error:&error];
					
					if (error != nil)
					{
						if (error.code == NSFileReadNoPermissionError)
						{
							return inCallback.Call(
												   kListDirectoryContentsStatus_PermissionDenied,
												   0,
												   "",
												   FileType::kUnknown);
						}
						LogFilePath("ListDirectoryContentsWithType: contentsOfDirectoryAtPath failed for path: ", inDirectoryPath);
						NOTIFY_ERROR("\terror:", [[error localizedDescription] UTF8String]);
						return inCallback.Call(kListDirectoryContentsStatus_Error, 0, "", FileType::kUnknown);
					}
					
					NSInteger numItems = [items count];
					if (numItems == 0)
					{
						return inCallback.Call(
											   kListDirectoryContentsStatus_DirectoryEmpty,
											   0,
											   "",
											   FileType::kUnknown);
					}
					else
					{
						for (NSInteger n = 0; n < numItems; ++n)
						{
							NSURL* oneURL = [items objectAtIndex:n];
							NSString* fileName = [oneURL lastPathComponent];
							
							NSNumber* isDirectory = nil;
							if (![oneURL getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:&error])
							{
								LogFilePath("CountDirectoryContentsWithSize: getResourceValue(NSURLIsDirectoryKey) failed for: ", inDirectoryPath);
								NOTIFY_ERROR("-- inItemName:", [fileName UTF8String]);
								return inCallback.Call(kListDirectoryContentsStatus_Error, 0, "", FileType::kUnknown);
							}
							NSNumber* isLink = nil;
							if (![oneURL getResourceValue:&isLink forKey:NSURLIsSymbolicLinkKey error:&error])
							{
								LogFilePath("CountDirectoryContentsWithSize: getResourceValue(NSURLIsSymbolicLinkKey) failed for: ", inDirectoryPath);
								NOTIFY_ERROR("-- inItemName:", [fileName UTF8String]);
								return inCallback.Call(kListDirectoryContentsStatus_Error, 0, "", FileType::kUnknown);
							}
							
							std::string fileNameUTF8([fileName UTF8String]);
							
							FileType fileType = FileType::kFile;
							if ([isLink boolValue])
							{
								fileType = FileType::kSymbolicLink;
							}
							else if ([isDirectory boolValue])
							{
								fileType = FileType::kDirectory;
							}
							
							for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n)
							{
								if (fileNameUTF8[n] == ':')
								{
									fileNameUTF8[n] = '/';
								}
							}
							bool keepGoing = inCallback.Call(
															 kListDirectoryContentsStatus_Success,
															 inDirectoryPath,
															 fileNameUTF8,
															 fileType);
							
							if (!keepGoing)
							{
								return false;
							}
							
							if (inDescendSubdirectories && (fileType == FileType::kDirectory))
							{
								FilePathCallbackClassT<FilePathPtr> filePathCallback;
								AppendToFilePath(inDirectoryPath, fileNameUTF8, filePathCallback);
								
								if (filePathCallback.mFilePath.P() == nullptr)
								{
									LogFilePath("ListDirectoryContentsWithType: AppendToFilePath for parent path: ", inDirectoryPath);
									NOTIFY_ERROR("-- file name:", fileNameUTF8);
									return inCallback.Call(kListDirectoryContentsStatus_Error, 0, "", FileType::kUnknown);
								}
								
								bool keepGoing = ProcessDirectory(
																  filePathCallback.mFilePath.P(),
																  inDescendSubdirectories,
																  inCallback);
								
								if (!keepGoing)
								{
									return false;
								}
							}
						}
					}
					return true;
				}
			};
#else
			
			//
			class Lister {
			public:
				//
				static ListDirectoryContentsResult ProcessDirectory(const HermitPtr& h_,
																	const FilePathPtr& inDirectoryPath,
																	const bool& inDescendSubdirectories,
																	const ListDirectoryContentsWithTypeItemCallbackRef& inItemCallback) {
					
					std::string pathUTF8;
					FilePathToCocoaPathString(h_, inDirectoryPath, pathUTF8);
					string::AddTrailingSlash(pathUTF8, pathUTF8);					
					NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
					
					NSError* error = nil;
					NSArray* items = [[NSFileManager defaultManager] contentsOfDirectoryAtPath:pathString error:&error];
					if (error != nil) {
						if (error.code == NSFileReadNoPermissionError) {
							return ListDirectoryContentsResult::kPermissionDenied;
						}

						NOTIFY_ERROR(h_,
									 "ListDirectoryContentsWithType: contentsOfDirectoryAtPath failed for path:",
									 inDirectoryPath,
									 "error:",
									 [[error localizedDescription] UTF8String]);
							
						return ListDirectoryContentsResult::kError;
					}
					
					NSInteger numItems = [items count];
					for (NSInteger n = 0; n < numItems; ++n) {
						NSString* fileName = [items objectAtIndex:n];
						
						std::string itemPathUTF8(pathUTF8);
						std::string fileNameUTF8([fileName cStringUsingEncoding:NSUTF8StringEncoding]);
						itemPathUTF8 += fileNameUTF8;
						
						FileType fileType = FileType::kFile;
						struct stat s;
						int err = lstat(itemPathUTF8.c_str(), &s);
						if (err != 0) {
							NOTIFY_ERROR(h_,
										 "ListDirectoryContentsWithType: lstat failed for path:", itemPathUTF8,
										 "err:", err);
							
							return ListDirectoryContentsResult::kError;
						}
						if (S_ISLNK(s.st_mode)) {
							fileType = FileType::kSymbolicLink;
						}
						else if (S_ISDIR(s.st_mode)) {
							fileType = FileType::kDirectory;
						}
						
						for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n) {
							if (fileNameUTF8[n] == ':') {
								fileNameUTF8[n] = '/';
							}
						}
						bool keepGoing = inItemCallback.Call(h_, inDirectoryPath, fileNameUTF8, fileType);
						if (!keepGoing) {
							return ListDirectoryContentsResult::kCanceled;
						}
						
						if (inDescendSubdirectories && (fileType == FileType::kDirectory)) {
							FilePathPtr filePath;
							AppendToFilePath(h_, inDirectoryPath, fileNameUTF8, filePath);
							if (filePath == nullptr) {
								NOTIFY_ERROR(h_,
											 "ListDirectoryContentsWithType: AppendToFilePath for parent path:",
											 inDirectoryPath,
											 "file name:",
											 fileNameUTF8);
								
								return ListDirectoryContentsResult::kError;
							}
							
							auto result = ProcessDirectory(h_, filePath, true, inItemCallback);
							if (result != ListDirectoryContentsResult::kSuccess) {
								return result;
							}
						}
					}
					return ListDirectoryContentsResult::kSuccess;
				}
			};
#endif
			
		} // private namespace
		
		//
		ListDirectoryContentsResult ListDirectoryContentsWithType(const HermitPtr& h_,
																  const FilePathPtr& inDirectoryPath,
																  const bool& inDescendSubdirectories,
																  const ListDirectoryContentsWithTypeItemCallbackRef& inItemCallback) {
			
			@autoreleasepool {
				return Lister::ProcessDirectory(h_, inDirectoryPath, inDescendSubdirectories, inItemCallback);
			}
		}
		
	} // namespace file
} // namespace hermit
