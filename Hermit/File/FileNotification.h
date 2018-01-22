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

#ifndef FileNotification_h
#define FileNotification_h

#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "FileType.h"

namespace hermit {
	namespace file {
		
		//
		extern const char* kFileErrorNotification;
		extern const char* kFilesMatchNotification;
		extern const char* kFilesDifferNotification;
		extern const char* kFileSkippedNotification;
		
		//
		extern const char* kErrorComparingFinderInfo;
		extern const char* kErrorReadingFinderInfo;
		extern const char* kErrorReadingXAttrs;
		
		//
		extern const char* kFilesMatch;
		extern const char* kFileTypesDiffer;
		extern const char* kFileSizesDiffer;
		extern const char* kFileContentsDiffer;
		extern const char* kFolderContentsDiffer;
		extern const char* kItemInPath1Only;
		extern const char* kItemInPath2Only;
		extern const char* kCreationDatesDiffer;
		extern const char* kModificationDatesDiffer;
		extern const char* kLockedStatesDiffer;
		extern const char* kBSDFlagsDiffer;
		extern const char* kACLsDiffer;
		extern const char* kPackageStatesDiffer;
		extern const char* kFinderInfosDiffer;
        extern const char* kXAttrPresenceMismatch;
        extern const char* kXAttrValuesDiffer;
		extern const char* kPermissionsDiffer;
		extern const char* kUserOwnersDiffer;
		extern const char* kGroupOwnersDiffer;
		extern const char* kLinkTargetsDiffer;
		extern const char* kIsAliasDiffers;
		extern const char* kAliasTargetsDiffer;
		extern const char* kIsHardLinkDiffers;
		extern const char* kHardLinkFileSystemNumDiffers;
		extern const char* kHardLinkFileNumDiffers;
		extern const char* kFileDeviceStatesDiffer;
		extern const char* kFileDeviceModesDiffer;
		extern const char* kFileDeviceIDsDiffer;
		extern const char* kFileSkipped;
		
		//
		struct FileNotificationParams {
			//
			FileNotificationParams(const std::string& inType, const FilePathPtr& inPath1);
			//
			FileNotificationParams(const std::string& inType, const FilePathPtr& inPath1, const FilePathPtr& inPath2);
			//
			FileNotificationParams(const std::string& inType,
								   const FilePathPtr& inPath1,
								   const FilePathPtr& inPath2,
								   const std::string& inString1,
								   const std::string& inString2);
			//
			FileNotificationParams(const std::string& inType,
								   const FilePathPtr& inPath1,
								   const FilePathPtr& inPath2,
								   uint64_t inInt1,
								   uint64_t inInt2);
			//
			std::string mType;
			FilePathPtr mPath1;
			FilePathPtr mPath2;
			std::string mString1;
			std::string mString2;
			uint64_t mInt1;
			uint64_t mInt2;
		};
		
		//
		extern const char* kBytesWrittenNotification;
		//
		struct BytesWrittenNotificationParam {
			//
			BytesWrittenNotificationParam(const FilePathPtr& path, uint64_t bytes);
			//
			FilePathPtr mPath;
			uint64_t mBytes;
		};
		
	} // namespace file
} // namespace hermit

#endif
