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

#include <Carbon/Carbon.h> // for FileInfo etc struct defines only, copied below for reference
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "GetFileXAttrs.h"
#include "PathIsDirectory.h"
#include "CompareFinderInfo.h"

#if 0

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


//	NOTE: FIELDS ARE BIG-ENDIAN!!!


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
#endif

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			//
			class GetFileXAttrsCallbackClass
			:
			public GetFileXAttrsCallback
			{
			public:
				//
				//
				GetFileXAttrsCallbackClass()
				:
				mResult(kGetFileXAttrsResult_Unknown)
				{
				}
				
				//
				//
				bool Function(
							  const GetFileXAttrsResult& inResult,
							  const std::string& inOneXAttrName,
							  const std::string& inOneXAttrData)
				{
					mResult = inResult;
					if (inResult == kGetFileXAttrsResult_Success)
					{
						if (inOneXAttrName == "com.apple.FinderInfo")
						{
							mFinderInfo.assign(inOneXAttrData);
							return false;
						}
					}
					return true;
				}
				
				//
				//
				GetFileXAttrsResult mResult;
				std::string mFinderInfo;
			};
			
		} // private namespace
		
		//
		CompareFinderInfoStatus CompareFinderInfo(const HermitPtr& h_, const FilePathPtr& inFilePath1, const FilePathPtr& inFilePath2) {
			GetFileXAttrsCallbackClass xattrs1;
			GetFileXAttrs(h_, inFilePath1, xattrs1);
			if (xattrs1.mResult != kGetFileXAttrsResult_Success) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: GetFileXAttrs failed for:", inFilePath1);
				return kCompareFinderInfoStatus_Error;
			}
			
			GetFileXAttrsCallbackClass xattrs2;
			GetFileXAttrs(h_, inFilePath2, xattrs2);
			if (xattrs2.mResult != kGetFileXAttrsResult_Success) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: GetFileXAttrs failed for:", inFilePath2);
				return kCompareFinderInfoStatus_Error;
			}
			
			bool path1IsDirectory = false;
			auto status1 = PathIsDirectory(h_, inFilePath1, path1IsDirectory);
			if (status1 != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: PathIsDirectory failed for:", inFilePath1);
				return kCompareFinderInfoStatus_Error;
			}
			
			bool path2IsDirectory = false;
			auto status2 = PathIsDirectory(h_, inFilePath2, path2IsDirectory);
			if (status2 != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: PathIsDirectory failed for:", inFilePath2);
				return kCompareFinderInfoStatus_Error;
			}
			
			if (path1IsDirectory != path2IsDirectory) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: is1Directory != is2Directory for path 1:", inFilePath1);
				NOTIFY_ERROR(h_, "-- path 2: ", inFilePath2);
				return kCompareFinderInfoStatus_Error;
			}
			
			if (xattrs1.mFinderInfo.size() != xattrs2.mFinderInfo.size()) {
				if (xattrs1.mFinderInfo.size() == 0) {
					if (path2IsDirectory && (xattrs2.mFinderInfo.size() == 32)) {
						FolderInfo* folder2Info = (FolderInfo*)&xattrs2.mFinderInfo.at(0);
						//	fields are big-endian!
						if ((htons(folder2Info->finderFlags) ^ kHasBundle) == 0) {
							return kCompareFinderInfoStatus_FolderPackageStatesDiffer;
						}
					}
				}
				else if (xattrs2.mFinderInfo.size() == 0) {
					if (path1IsDirectory && (xattrs1.mFinderInfo.size() == 32)) {
						FolderInfo* folder1Info = (FolderInfo*)&xattrs1.mFinderInfo.at(0);
						//	fields are big-endian!
						if ((htons(folder1Info->finderFlags) ^ kHasBundle) == 0) {
							return kCompareFinderInfoStatus_FolderPackageStatesDiffer;
						}
					}
				}
				return kCompareFinderInfoStatus_FinderInfosDiffer;
			}
			if (xattrs1.mFinderInfo.size() == 0) {
				return kCompareFinderInfoStatus_Match;
			}
			
			if (xattrs1.mFinderInfo.size() != 32) {
				NOTIFY_ERROR(h_, "CompareFinderInfo: Unexpected finder info size for: ", inFilePath1);
				return kCompareFinderInfoStatus_Error;
			}
			
			//	we ignore:
			//		location
			//		windowBounds
			//		putAwayFolderID
			//		scrollPosition
			//	as immaterial for compare purposes
			//	and of course any "reserved" fields, which should be 0 anyway
			
			if (path1IsDirectory) {
				FolderInfo* folder1Info = (FolderInfo*)&xattrs1.mFinderInfo.at(0);
				FolderInfo* folder2Info = (FolderInfo*)&xattrs2.mFinderInfo.at(0);
				if (folder1Info->finderFlags != folder2Info->finderFlags) {
					//	fields are big-endian!
					if ((htons(folder1Info->finderFlags) | kHasBundle) == (htons(folder2Info->finderFlags) | kHasBundle)) {
						return kCompareFinderInfoStatus_FolderPackageStatesDiffer;
					}
					
					//			NOTIFY_ERROR("CompareFinderInfo: folderInfos differ for path 1: ", inFilePath1);
					//			NOTIFY_ERROR("-- path 2: ", inFilePath2);
					//			LogSInt32("-- folder1Info->finderFlags= ", folder1Info->finderFlags);
					//			LogSInt32("-- folder1Info->finderFlags= ", folder2Info->finderFlags);
					
					return kCompareFinderInfoStatus_FinderInfosDiffer;
				}
				ExtendedFolderInfo* folder1ExtInfo = (ExtendedFolderInfo*)&xattrs1.mFinderInfo.at(16);
				ExtendedFolderInfo* folder2ExtInfo = (ExtendedFolderInfo*)&xattrs2.mFinderInfo.at(16);
				if (folder1ExtInfo->extendedFinderFlags != folder2ExtInfo->extendedFinderFlags) {
					//			NOTIFY_ERROR("CompareFinderInfo: folderExtInfos differ for path 1: ", inFilePath1);
					//			NOTIFY_ERROR("-- path 2: ", inFilePath2);
					//			LogSInt32("-- folder1ExtInfo->extendedFinderFlags= ", folder1ExtInfo->extendedFinderFlags);
					//			LogSInt32("-- folder2ExtInfo->extendedFinderFlags= ", folder2ExtInfo->extendedFinderFlags);
					
					return kCompareFinderInfoStatus_FinderInfosDiffer;
				}
			}
			else
			{
				FileInfo* file1FileInfo = (FileInfo*)&xattrs1.mFinderInfo.at(0);
				FileInfo* file2FileInfo = (FileInfo*)&xattrs2.mFinderInfo.at(0);
				if ((file1FileInfo->fileType != file2FileInfo->fileType) ||
					(file1FileInfo->fileCreator != file2FileInfo->fileCreator) ||
					(file1FileInfo->finderFlags != file2FileInfo->finderFlags))
				{
					//			NOTIFY_ERROR("CompareFinderInfo: fileFinderInfos differ for path 1: ", inFilePath1);
					//			NOTIFY_ERROR("-- path 2: ", inFilePath2);
					//			LogSInt32("-- file1FileInfo->fileType= ", file1FileInfo->fileType);
					//			LogSInt32("-- file1FileInfo->fileCreator= ", file1FileInfo->fileCreator);
					//			LogSInt32("-- file1FileInfo->finderFlags= ", file1FileInfo->finderFlags);
					//			LogSInt32("-- file2FileInfo->fileType= ", file2FileInfo->fileType);
					//			LogSInt32("-- file2FileInfo->fileCreator= ", file2FileInfo->fileCreator);
					//			LogSInt32("-- file2FileInfo->finderFlags= ", file2FileInfo->finderFlags);
					
					return kCompareFinderInfoStatus_FinderInfosDiffer;
				}
				ExtendedFileInfo* file1ExtInfo = (ExtendedFileInfo*)&xattrs1.mFinderInfo.at(16);
				ExtendedFileInfo* file2ExtInfo = (ExtendedFileInfo*)&xattrs2.mFinderInfo.at(16);
				if (file1ExtInfo->extendedFinderFlags != file2ExtInfo->extendedFinderFlags)
				{
					//			NOTIFY_ERROR("CompareFinderInfo: fileExtInfos differ for path 1: ", inFilePath1);
					//			NOTIFY_ERROR("-- path 2: ", inFilePath2);
					//			LogSInt32("-- file1ExtInfo->extendedFinderFlags= ", file1ExtInfo->extendedFinderFlags);
					//			LogSInt32("-- file2ExtInfo->extendedFinderFlags= ", file2ExtInfo->extendedFinderFlags);
					
					return kCompareFinderInfoStatus_FinderInfosDiffer;
				}
			}
			
			return kCompareFinderInfoStatus_Match;
		}
		
	} // namespace file
} // namespace hermit

