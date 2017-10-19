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

#import <AppKit/NSWorkspace.h>
#import <Foundation/Foundation.h>
#import <string>
#import "FilePathToCocoaPathString.h"
#import "PathIsPackage.h"

namespace hermit {
	namespace file {
		
		//
		void PathIsPackage(const HermitPtr& h_, const FilePathPtr& inFilePath, const PathIsPackageCallbackRef& inCallback) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				BOOL isPackage = [[NSWorkspace sharedWorkspace] isFilePackageAtPath:pathString];
				if (isPackage == YES) {
					inCallback.Call(inFilePath, true, true);
				}
				else {
					inCallback.Call(inFilePath, true, false);
				}
			}
		}
		
	} // namespace file
} // namespace hermit
