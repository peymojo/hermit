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
#import <string>
#import <sys/stat.h>
#import "Hermit/Foundation/Notification.h"
#import "Hermit/String/AddTrailingSlash.h"
#import "AppendToFilePath.h"
#import "FilePathToCocoaPathString.h"
#import "CountDirectoryContentsWithSize.h"

namespace hermit {
	namespace file {
		
		namespace {

			//
			CountDirectoryContentsWithSizeStatus ProcessDirectory(const HermitPtr& h_,
																  const FilePathPtr& inDirectoryPath,
																  const bool& inDescendSubdirectories,
																  const bool& inIgnorePermissionsErrors,
																  const PreprocessFileFunctionPtr& inPreprocessFunction,
																  uint64_t& outCount,
																  uint64_t& outSize) {
				if (CHECK_FOR_ABORT(h_)) {
					return kCountDirectoryContentsWithSizeStatus_Canceled;
				}
				
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inDirectoryPath, pathUTF8);
				string::AddTrailingSlash(pathUTF8, pathUTF8);
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				NSURL* url = [NSURL fileURLWithPath:pathString];
				NSArray* keys = @[ NSURLIsDirectoryKey, NSURLIsSymbolicLinkKey, NSURLFileSizeKey ];
				NSError* error = nil;
				NSArray* items = [[NSFileManager defaultManager]
								  contentsOfDirectoryAtURL:url
								  includingPropertiesForKeys:keys
								  options:0
								  error:&error];
				
				if (error != nil) {
					NSInteger errorCode = [error code];
					if (errorCode == NSFileReadNoSuchFileError) {
						return kCountDirectoryContentsWithSizeStatus_DirectoryNotFound;
					}
					if (errorCode == NSFileReadNoPermissionError) {
						if (inIgnorePermissionsErrors) {
							outCount = 1;
							return kCountDirectoryContentsWithSizeStatus_Success;
						}
						return kCountDirectoryContentsWithSizeStatus_PermissionDenied;
					}
					
					NOTIFY_ERROR(h_,
								 "CountDirectoryContentsWithSize: contentsOfDirectoryAtPath failed for path:", inDirectoryPath,
								 "-- error:", [[error localizedDescription] UTF8String],
								 "-- error code:", (int32_t)errorCode);
					return kCountDirectoryContentsWithSizeStatus_Error;
				}
				
				NSInteger numItems = [items count];
				if (numItems == 0) {
					outCount = 0;
					return kCountDirectoryContentsWithSizeStatus_Success;
				}
				
				uint64_t count = numItems;
				uint64_t size = 0;
				for (NSInteger n = 0; n < numItems; ++n) {
					NSURL* url = [items objectAtIndex:n];
					NSString* fileName = [url lastPathComponent];
					
					if (((n % 1000) == 999) && CHECK_FOR_ABORT(h_)) {
						return kCountDirectoryContentsWithSizeStatus_Canceled;
					}
					
					if (inPreprocessFunction != nullptr) {
						auto instruction = inPreprocessFunction->Preprocess(h_, inDirectoryPath, [fileName UTF8String]);
						if (instruction == PreprocessFileInstruction::kSkip) {
							count--;
							continue;
						}
						if (instruction != PreprocessFileInstruction::kContinue) {
							NOTIFY_ERROR(h_,
										 "CountDirectoryContentsWithSize: PreprocessFunction failed for:", inDirectoryPath,
										 "fileName:", [fileName UTF8String]);
							return kCountDirectoryContentsWithSizeStatus_Error;
						}
					}
					
					NSNumber* fileSize = nil;
					if (![url getResourceValue:&fileSize forKey:NSURLFileSizeKey error:&error]) {
						NOTIFY_ERROR(h_,
									 "CountDirectoryContentsWithSize: getResourceValue failed for:", inDirectoryPath,
									 "key: NSURLFileSizeKey",
									 "fileName:", [fileName UTF8String]);
						return kCountDirectoryContentsWithSizeStatus_Error;
					}
					size += [fileSize longLongValue];
					
					if (inDescendSubdirectories) {
						NSNumber* isDirectory = nil;
						if (![url getResourceValue:&isDirectory forKey:NSURLIsDirectoryKey error:&error]) {
							NOTIFY_ERROR(h_,
										 "CountDirectoryContentsWithSize: getResourceValue failed for:", inDirectoryPath,
										 "key: NSURLIsDirectoryKey",
										 "fileName:", [fileName UTF8String]);
							return kCountDirectoryContentsWithSizeStatus_Error;
						}
						NSNumber* isLink = nil;
						if (![url getResourceValue:&isLink forKey:NSURLIsSymbolicLinkKey error:&error]) {
							NOTIFY_ERROR(h_,
										 "CountDirectoryContentsWithSize: getResourceValue failed for:", inDirectoryPath,
										 "key: NSURLIsSymbolicLinkKey",
										 "fileName:", [fileName UTF8String]);
							return kCountDirectoryContentsWithSizeStatus_Error;
						}
						
						if ([isDirectory boolValue] && ![isLink boolValue]) {
							std::string fileNameUTF8([fileName cStringUsingEncoding:NSUTF8StringEncoding]);
							for (std::string::size_type n = 0; n < fileNameUTF8.size(); ++n) {
								if (fileNameUTF8[n] == ':') {
									fileNameUTF8[n] = '/';
								}
							}
							
							FilePathPtr filePath;
							AppendToFilePath(h_, inDirectoryPath, fileNameUTF8, filePath);
							if (filePath == nullptr) {
								NOTIFY_ERROR(h_,
											 "CountDirectoryContentsWithSize: AppendToFilePath failed for path:", inDirectoryPath,
											"leaf name:", fileNameUTF8);
								return kCountDirectoryContentsWithSizeStatus_Error;
							}
							
							uint64_t subDirCount = 0;
							uint64_t subDirSize = 0;
							CountDirectoryContentsWithSizeStatus status = ProcessDirectory(h_,
																						   filePath,
																						   inDescendSubdirectories,
																						   inIgnorePermissionsErrors,
																						   inPreprocessFunction,
																						   subDirCount,
																						   subDirSize);
							
							if (status != kCountDirectoryContentsWithSizeStatus_Success) {
								return status;
							}
							count += subDirCount;
							size += subDirSize;
						}
					}
				}
				outCount = count;
				outSize = size;
				return kCountDirectoryContentsWithSizeStatus_Success;
			}
			
		} // private namespace
		
		//
		void CountDirectoryContentsWithSize(const HermitPtr& h_,
											const FilePathPtr& inDirectoryPath,
											const bool& inDescendSubdirectories,
											const bool& inIgnorePermissionsErrors,
											const PreprocessFileFunctionPtr& inPreprocessFunction,
											const CountDirectoryContentsWithSizeCallbackRef& inCallback) {
			@autoreleasepool {
				uint64_t count = 0;
				uint64_t size = 0;
				CountDirectoryContentsWithSizeStatus status = ProcessDirectory(h_,
																			   inDirectoryPath,
																			   inDescendSubdirectories,
																			   inIgnorePermissionsErrors,
																			   inPreprocessFunction,
																			   count,
																			   size);
				
				inCallback.Call(status, count, size);
			}
		}
		
	} // namespace file
} // namespace hermit
