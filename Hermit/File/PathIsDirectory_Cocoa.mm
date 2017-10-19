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
#import "FilePathToCocoaPathString.h"
#import "PathIsDirectory.h"

namespace hermit {
	namespace file {
		
		//
		PathIsDirectoryStatus PathIsDirectory(const HermitPtr& h_, const FilePathPtr& path, bool& outIsDirectory) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, path, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				
				//		this is slower?
				//
				//		NSError* error = nil;
				//		NSDictionary* dict = [[NSFileManager defaultManager] attributesOfItemAtPath:pathString error:&error];
				//		NSString* fileType = [dict fileType];
				////	NSLog(@"fileType: %@", fileType);
				//		if ([fileType isEqualToString:NSFileTypeDirectory])
				//		{
				//			inCallback.Call(inHub, inFilePath, true, true);
				//		}
				//		else
				//		{
				//			inCallback.Call(inHub, inFilePath, true, false);
				//		}
				
				BOOL isDirectory = NO;
				BOOL exists = [[NSFileManager defaultManager] fileExistsAtPath:pathString isDirectory:&isDirectory];
				if ((exists == YES) && (isDirectory == YES)) {
					outIsDirectory = true;
					return PathIsDirectoryStatus::kSuccess;
				}
			}
			outIsDirectory = false;
			return PathIsDirectoryStatus::kSuccess;
		}
		
	} // namespace file
} // namespace hermit
