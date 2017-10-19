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
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "GetFileResourceValues.h"

namespace hermit {
	namespace file {
		
#if 0
		
		struct FileInfo {
			OSType              fileType;               /* The type of the file */
			OSType              fileCreator;            /* The file's creator */
			UInt16              finderFlags;            /* ex: kHasBundle, kIsInvisible... */
			Point               location;               /* File's location in the folder */
			/* If set to {0, 0}, the Finder will place the item automatically */
			UInt16              reservedField;          /* (set to 0) */
		};
		typedef struct FileInfo                 FileInfo;
		struct FolderInfo {
			Rect                windowBounds;           /* The position and dimension of the folder's window */
			UInt16              finderFlags;            /* ex. kIsInvisible, kNameLocked, etc.*/
			Point               location;               /* Folder's location in the parent folder */
			/* If set to {0, 0}, the Finder will place the item automatically */
			UInt16              reservedField;          /* (set to 0) */
		};
		typedef struct FolderInfo               FolderInfo;
		struct ExtendedFileInfo {
			SInt16              reserved1[4];           /* Reserved (set to 0) */
			UInt16              extendedFinderFlags;    /* Extended flags (custom badge, routing info...) */
			SInt16              reserved2;              /* Reserved (set to 0). Comment ID if high-bit is clear */
			SInt32              putAwayFolderID;        /* Put away folder ID */
		};
		typedef struct ExtendedFileInfo         ExtendedFileInfo;
		struct ExtendedFolderInfo {
			Point               scrollPosition;         /* Scroll position (for icon views) */
			SInt32              reserved1;              /* Reserved (set to 0) */
			UInt16              extendedFinderFlags;    /* Extended flags (custom badge, routing info...) */
			SInt16              reserved2;              /* Reserved (set to 0). Comment ID if high-bit is clear */
			SInt32              putAwayFolderID;        /* Put away folder ID */
		};
		typedef struct ExtendedFolderInfo       ExtendedFolderInfo;
		
		
		FileInfo.fileType
		--> attributesOfItemAtPath . - (OSType)fileHFSTypeCode;
		FileInfo.fileCreator
		--> attributesOfItemAtPath . - (OSType)fileHFSCreatorCode;
		FileInfo.finderFalgs
		--> ???
		
		/* Finder flags (finderFlags, fdFlags and frFlags) */
		/* Any flag reserved or not specified should be set to 0. */
		/* If a flag applies to a file, but not to a folder, make sure to check */
		/* that the item is not a folder by checking ((ParamBlockRec.ioFlAttrib & ioDirMask) == 0) */
		enum {
			kIsOnDesk                     = 0x0001, /* Files and folders (System 6) */
			kColor                        = 0x000E, /* Files and folders */
			/* bit 0x0020 was kRequireSwitchLaunch, but is now reserved for future use*/
			kIsShared                     = 0x0040, /* Files only (Applications only) */
			/* If clear, the application needs to write to */
			/* its resource fork, and therefore cannot be */
			/* shared on a server */
			kHasNoINITs                   = 0x0080, /* Files only (Extensions/Control Panels only) */
			/* This file contains no INIT resource */
			kHasBeenInited                = 0x0100, /* Files only */
			/* Clear if the file contains desktop database */
			/* resources ('BNDL', 'FREF', 'open', 'kind'...) */
			/* that have not been added yet. Set only by the Finder */
			/* Reserved for folders - make sure this bit is cleared for folders */
			/* bit 0x0200 was the letter bit for AOCE, but is now reserved for future use */
			kHasCustomIcon                = 0x0400, /* Files and folders */
			kIsStationery                 = 0x0800, /* Files only */
			kNameLocked                   = 0x1000, /* Files and folders */
			kHasBundle                    = 0x2000, /* Files and folders */
			/* Indicates that a file has a BNDL resource */
			/* Indicates that a folder is displayed as a package */
			kIsInvisible                  = 0x4000, /* Files and folders */
			kIsAlias                      = 0x8000 /* Files only */
		};
		
		
		FileInfo.location								"File's location in the folder"
		--> ???
		
		ExtendedFileInfo.extendedFinderFlags
		--> ???
		
		/* Extended flags (extendedFinderFlags, fdXFlags and frXFlags) */
		/* Any flag not specified should be set to 0. */
		enum {
			kExtendedFlagsAreInvalid      = 0x8000, /* If set the other extended flags are ignored */
			kExtendedFlagHasCustomBadge   = 0x0100, /* Set if the file or folder has a badge resource */
			kExtendedFlagObjectIsBusy     = 0x0080, /* Set if the object is marked as busy/incomplete */
			kExtendedFlagHasRoutingInfo   = 0x0004 /* Set if the file contains routing info resource */
		};
		
		
		ExtendedFileInfo.putAwayFolderID				( used to return items to their original location from the trash? )
		--> ???
		
		FolderInfo.windowBounds
		--> ???
		FolderInfo.finderFlags
		--> ???
		FolderInfo.location								"Folder's location in the parent folder"
		--> ???
		
		ExtendedFolderInfo.scrollPosition
		--> ???
		ExtendedFolderInfo.extendedFinderFlags
		--> ???
		ExtendedFolderInfo.putAwayFolderID				( used to return items to their original location from the trash? )
		--> ???
		
#endif
		
		
		
		//
		void GetFileResourceValues(const HermitPtr& h_,
								   const FilePathPtr& inFilePath,
								   const GetFileResourceValuesCallbackRef& inCallback) {
			@autoreleasepool {
				std::string pathUTF8;
				FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
				
				NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
				NSURL* fileUrl = [NSURL fileURLWithPath:pathString];
				
				NSArray* keys = nil;
				NSError* error = nil;
				NSDictionary* dict = [fileUrl resourceValuesForKeys:keys error:&error];
				if (dict == nil) {
					NOTIFY_ERROR(h_, "GetFileResourceValues: resourceValuesForKeys failed for path:", inFilePath);
					if (error != nil) {
						NOTIFY_ERROR(h_, "\terror:", [[error localizedDescription] UTF8String]);
					}
					inCallback.Call(false, 0);
				}
				else {
					//			uint32_t permissions = (uint32_t)[dict filePosixPermissions];
					//			inCallback.Call(inHub, true, permissions);
				}
			}
		}
		
	} // namespace file
} // namespace hermit
