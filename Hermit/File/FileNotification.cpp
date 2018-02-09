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

#include <string.h>
#include "FileNotification.h"

namespace hermit {
	namespace file {
		
		//
		const char* kFileErrorNotification = "fileerror";
		const char* kFilesMatchNotification = "filesmatch";
		const char* kFilesDifferNotification = "filesdiffer";
		const char* kFileSkippedNotification = "fileskipped";
		
		//
		const char* kErrorComparingFinderInfo = "finderinfocompareerror";
		const char* kErrorReadingFinderInfo = "finderinforeaderror";
		const char* kErrorReadingXAttrs = "xattrsreaderror";
		
		//
		const char* kFilesMatch = "filesmatch";
		const char* kFileTypesDiffer = "filetypesdiffer";
		const char* kFileSizesDiffer = "filesizesdiffer";
		const char* kFileContentsDiffer = "filecontentsdiffer";
		const char* kFolderContentsDiffer = "foldercontentsdiffer";
		const char* kItemInPath1Only = "iteminpath1only";
		const char* kItemInPath2Only = "iteminpath2only";
		const char* kCreationDatesDiffer = "creationdatesdiffer";
		const char* kModificationDatesDiffer = "modificationdatesdiffer";
		const char* kLockedStatesDiffer = "lockedstatesdiffer";
		const char* kBSDFlagsDiffer = "bsdflagsdiffer";
		const char* kACLsDiffer = "aclsdiffer";
		const char* kPackageStatesDiffer = "packagestatesdiffer";
		const char* kFinderInfosDiffer = "finderinfosdiffer";
        const char* kXAttrPresenceMismatch = "xattr-presence-mismatch";
        const char* kXAttrValuesDiffer = "xattr-values-differ";
        const char* kPermissionsDiffer = "permissionsdiffer";
		const char* kUserOwnersDiffer = "userownersdiffer";
		const char* kGroupOwnersDiffer = "groupownersdiffer";
		const char* kLinkTargetsDiffer = "linktargetsdiffer";
		const char* kIsAliasDiffers = "isaliasdiffers";
		const char* kAliasTargetsDiffer = "aliastargetsdiffer";
		const char* kIsHardLinkDiffers = "ishardlinkdiffers";
		const char* kHardLinksDiffer = "hardlinksdiffer";
		const char* kFileDeviceStatesDiffer = "devicestatesdiffer";
		const char* kFileDeviceModesDiffer = "devicemodesdiffer";
		const char* kFileDeviceIDsDiffer = "deviceidsdiffer";
		const char* kFileSkipped = "fileskipped";
		
		//
		FileNotificationParams::FileNotificationParams(const std::string& inType, const FilePathPtr& inPath1) :
		mType(inType),
		mPath1(inPath1),
		mPath2(NULL),
		mString1(""),
		mString2(""),
		mInt1(0),
		mInt2(0) {
		}
		
		//
		FileNotificationParams::FileNotificationParams(const std::string& inType,
													   const FilePathPtr& inPath1,
													   const FilePathPtr& inPath2) :
		mType(inType),
		mPath1(inPath1),
		mPath2(inPath2),
		mString1(""),
		mString2(""),
		mInt1(0),
		mInt2(0) {
		}
		
		//
		FileNotificationParams::FileNotificationParams(const std::string& inType,
													   const FilePathPtr& inPath1,
													   const FilePathPtr& inPath2,
													   const std::string& inString1,
													   const std::string& inString2) :
		mType(inType),
		mPath1(inPath1),
		mPath2(inPath2),
		mString1(inString1),
		mString2(inString2),
		mInt1(0),
		mInt2(0) {
		}
		
		//
		FileNotificationParams::FileNotificationParams(const std::string& inType,
													   const FilePathPtr& inPath1,
													   const FilePathPtr& inPath2,
													   uint64_t inInt1,
													   uint64_t inInt2) :
		mType(inType),
		mPath1(inPath1),
		mPath2(inPath2),
		mString1(""),
		mString2(""),
		mInt1(inInt1),
		mInt2(inInt2) {
		}
		
		//
		const char* kBytesWrittenNotification = "lib.file.byteswritten";
		
		//
		BytesWrittenNotificationParam::BytesWrittenNotificationParam(const FilePathPtr& path, uint64_t bytes) :
		mPath(path),
		mBytes(bytes) {
		}
		
	} // namespace file
} // namespace hermit
