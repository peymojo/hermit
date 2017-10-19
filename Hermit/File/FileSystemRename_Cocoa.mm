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
#import <Foundation/FoundationErrors.h>
#import <string>
#import "AppendToFilePath.h"
#import "FilePathToCocoaPathString.h"
#import "GetFilePathParent.h"
#import "FileSystemRename.h"

#if 000

namespace e38
{

//
//
void FileSystemRename(
	const ConstHubPtr& inHub,
	const FilePathPtr& inFilePath,
	const std::string& inNewFilename,
	const bool& inOverwriteExisting,
	const FileSystemRenameCallbackRef& inCallback)
{
	NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

	FilePathCallbackClassT<FilePathPtr> parentPath;
	GetFilePathParent(inHub, inFilePath, parentPath);
	if (parentPath.mFilePath.P() == 0)
	{
		LogFilePath(
			inHub,
			"FileSystemRename(): GetFilePathParent failed for path: ",
			inFilePath);
			
		inCallback.Call(inHub, kFileSystemRenameStatus_Error);
	}
	else
	{
		FilePathCallbackClassT<FilePathPtr> destPath;
		AppendToFilePath(inHub, parentPath.mFilePath.P(), inNewFilename, destPath);
		if (destPath.mFilePath.P() == 0)
		{
			LogFilePath(
				inHub,
				"FileSystemRename(): AppendToFilePath failed for parent path: ",
				parentPath.mFilePath.P());

			LogString(
				inHub,
				"-- and filename: ",
				inNewFilename);
				
			inCallback.Call(inHub, kFileSystemRenameStatus_Error);
			[pool release];
			return;
		}

		typedef StringCallbackClass StringCallback;
		StringCallback stringCallback;
		FilePathToCocoaPathString(inHub, inFilePath, stringCallback);
		std::string sourcePathUTF8(stringCallback.mValue);

		NSString* sourcePathString = [NSString stringWithUTF8String:sourcePathUTF8.c_str()];

		FilePathToCocoaPathString(inHub, destPath.mFilePath.P(), stringCallback);
		std::string destPathUTF8(stringCallback.mValue);

		if (destPathUTF8 == sourcePathUTF8)
		{
			LogFilePath(
				inHub,
				"FileSystemRename(): Old and new filenames are equal for path: ",
				inFilePath);
			LogString(
				inHub,
				"-- and new filename: ",
				inNewFilename);
				
			inCallback.Call(inHub, kFileSystemRenameStatus_Error);
			[pool release];
			return;
		}

		NSString* destPathString = [NSString stringWithUTF8String:destPathUTF8.c_str()];
		
		NSError* error = nil;
		NSDictionary* attributes = [[NSFileManager defaultManager] attributesOfItemAtPath:destPathString error:&error];
		if (attributes != nil)
		{
			if (!inOverwriteExisting)
			{
				inCallback.Call(inHub, kFileSystemRenameStatus_TargetAlreadyExists);
				[pool release];
				return;
			}
			
			BOOL success = [[NSFileManager defaultManager] removeItemAtPath:destPathString error:&error];
			if (!success)
			{
				LogString(
					inHub,
					"FileSystemRename(): Couldn't delete target item at path: ",
					destPathUTF8.c_str());
					
				inCallback.Call(inHub, kFileSystemRenameStatus_Error);
				[pool release];
				return;
			}
		}
		
		BOOL success = [[NSFileManager defaultManager] moveItemAtPath:sourcePathString toPath:destPathString error:&error];
		if (!success)
		{
			if (error != nil)
			{
				NSInteger errorCode = [error code];
//				NSString* errorDomain = [error domain];
				if (errorCode == NSFileReadNoSuchFileError)
				{
					LogFilePath(
						inHub,
						"FileSystemRename(): errorCode == NSFileReadNoSuchFileError for file at path: ",
						inFilePath);
						
					inCallback.Call(inHub, kFileSystemRenameStatus_SourceNotFound);
					[pool release];
					return;
				}

				LogFilePath(
					inHub,
					"FileSystemRename(): NSFileManager defaultManager] moveItemAtPath failed for file at path: ",
					inFilePath);
				LogSInt32(
					inHub,
					"\terror code: ",
					(int32_t)errorCode);
			}
			else
			{
				LogFilePath(
					inHub,
					"FileSystemRename(): NSFileManager defaultManager] moveItemAtPath failed for an unknown reason, for file at path: ",
					inFilePath);
			}
			
			inCallback.Call(inHub, kFileSystemRenameStatus_Error);
			[pool release];
			return;
		}
		inCallback.Call(inHub, kFileSystemRenameStatus_Success);
	}
	
    [pool release];
}

} // namespace e38

#endif
