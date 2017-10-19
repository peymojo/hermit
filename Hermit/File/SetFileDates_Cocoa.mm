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
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "PathIsSymbolicLink.h"
#import "SetFileDates.h"

#if !__has_feature(objc_arc)

#define AUTORELEASE(x) [x autorelease]
#define RETAIN(x) [x retain]
#define RELEASE(x) [x release]

#else

#define AUTORELEASE(x) x
#define RETAIN(x)
#define RELEASE(x)

#endif

namespace hermit {
	namespace file {
		
		//
		bool SetFileDates(const HermitPtr& h_,
						  const FilePathPtr& inFilePath,
						  const std::string& inCreationDate,
						  const std::string& inModificationDate) {
			
			@autoreleasepool {
				NSDateFormatter* dateFormatter = AUTORELEASE([[NSDateFormatter alloc] init]);
				[dateFormatter setDateFormat:@"yyyy-MM-dd HH:mm:ss Z"];
				
				BOOL result = YES;
				
				NSDate* creationDate = nil;
				if (!inCreationDate.empty()) {
					NSString* creationDateString = [NSString stringWithUTF8String:inCreationDate.c_str()];
					if (creationDateString == nil) {
						NOTIFY_ERROR(h_,
									 "SetFileDates: NSString stringWithUTF8String failed for creation date:",
									 inCreationDate);
						result = NO;
					}
					else {
						creationDate = [dateFormatter dateFromString:creationDateString];
						if (creationDate == nil) {
							NOTIFY_ERROR(h_,
										 "SetFileDates: dateFormatter dateFromString failed for creation date:",
										 inCreationDate);
							result = NO;
						}
					}
				}
				
				NSDate* modificationDate = nil;
				if ((result == YES) && (!inModificationDate.empty())) {
					NSString* modificationDateString = [NSString stringWithUTF8String:inModificationDate.c_str()];
					if (modificationDateString == nil) {
						NOTIFY_ERROR(h_,
									 "SetFileDates: NSString stringWithUTF8String failed for modification date:",
									 inModificationDate);
						result = NO;
					}
					else {
						modificationDate = [dateFormatter dateFromString:modificationDateString];
						if (modificationDate == nil) {
							NOTIFY_ERROR(h_,
										 "SetFileDates: dateFormatter dateFromString failed for modification date:",
										 inModificationDate);
							result = NO;
						}
					}
				}
				
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				if ((result == YES) && ((creationDate != nil) || (modificationDate != nil))) {
					NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:pathUTF8.c_str()]];
					
					NSError* error = nil;
					if (modificationDate != nil) {
						[url setResourceValue:modificationDate forKey:NSURLContentModificationDateKey error:&error];
						if (error != nil) {
							NOTIFY_ERROR(h_,
										 "SetFileDates: setResourceValue:NSURLContentModificationDateKey failed for file:",
										 inFilePath,
										 "error.localizedDescription:",
										 [error.localizedDescription UTF8String],
										 "error.domain:",
										 [error.domain UTF8String],
										 "error.code:",
										 (int32_t)error.code);
							
							result = NO;
						}
					}
					
					if ((error == nil) && (creationDate != nil)) {
						[url setResourceValue:creationDate forKey:NSURLCreationDateKey error:&error];
						if (error != nil) {
							NOTIFY_ERROR(h_,
										 "SetFileDates: setResourceValue:NSURLCreationDateKey failed for file:",
										 inFilePath,
										 "error.localizedDescription:",
										 [error.localizedDescription UTF8String],
										 "error.domain:",
										 [error.domain UTF8String],
										 "error.code:",
										 (int32_t)error.code);
							
							result = NO;
						}
					}
				}
				
				return (result == YES);
			}
		}
		
	} // namespace file
} // namespace hermit
