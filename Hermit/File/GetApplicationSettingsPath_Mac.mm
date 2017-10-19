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
#import "CreateFilePathFromUTF8String.h"
#import "GetApplicationSettingsPath.h"

namespace hermit {
	namespace file {
		
		//
		void GetApplicationSettingsPath(const std::string& applicationName, FilePathPtr& outSettingsPath) {
			@autoreleasepool {
				NSString* libraryDir = [NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES) lastObject];
				//NSLog(@"Library dir: %@", libraryDir);
				NSString* appPrefsDir = [NSString stringWithFormat:@"%@/%s", libraryDir, applicationName.c_str()];
				//NSLog(@"App prefs dir: %@", appPrefsDir);
				
				CreateFilePathFromUTF8String([appPrefsDir cStringUsingEncoding:NSUTF8StringEncoding], outSettingsPath);
			}
		}
		
	} // namespace file
} // namespace hermit
