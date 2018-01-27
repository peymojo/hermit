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

#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/GetRelativePath.h"
#include "CompareXAttrs.h"
#include "FileNotification.h"
#include "GetFileDates.h"
#include "GetFilePathUTF8String.h"
#include "GetFilePosixOwnership.h"
#include "GetFilePosixPermissions.h"
#include "GetSymbolicLinkTarget.h"
#include "CompareLinks.h"

namespace hermit {
	namespace file {
		
		//
		CompareLinksStatus CompareLinks(const HermitPtr& h_, const FilePathPtr& inFilePath1, const FilePathPtr& inFilePath2, const bool& inIgnoreDates) {
			std::string link1PathUTF8;
			GetFilePathUTF8String(h_, inFilePath1, link1PathUTF8);
			
			GetSymbolicLinkTargetCallbackClassT<FilePathPtr> link1TargetCallback;
			GetSymbolicLinkTarget(h_, inFilePath1, link1TargetCallback);
			if (!link1TargetCallback.mSuccess) {
				NOTIFY_ERROR(h_, "CompareLinks: GetSymbolicLinkTarget failed for path: ", inFilePath1);
				return kCompareLinksStatus_Error;
			}
			
			std::string link2PathUTF8;
			GetFilePathUTF8String(h_, inFilePath2, link2PathUTF8);
			
			GetSymbolicLinkTargetCallbackClassT<FilePathPtr> link2TargetCallback;
			GetSymbolicLinkTarget(h_, inFilePath2, link2TargetCallback);
			if (!link2TargetCallback.mSuccess) {
				NOTIFY_ERROR(h_, "CompareLinks: GetSymbolicLinkTarget failed for path: ", inFilePath2);
				return kCompareLinksStatus_Error;
			}
			
			std::string link1TargetPathUTF8;
			GetFilePathUTF8String(h_, link1TargetCallback.mFilePath, link1TargetPathUTF8);
			
			std::string link2TargetPathUTF8;
			GetFilePathUTF8String(h_, link2TargetCallback.mFilePath, link2TargetPathUTF8);
			
			if (link1TargetPathUTF8 != link2TargetPathUTF8) {
				std::string originalLink1TargetPathUTF8(link1TargetPathUTF8);
				if ((link1PathUTF8.find("/Volumes/") == 0) &&
					(link1TargetPathUTF8.find("/Volumes/") == 0)) {
					link1PathUTF8 = link1PathUTF8.substr(8);
					link1TargetPathUTF8 = link1TargetPathUTF8.substr(8);
				}
				
				std::string link1TargetRelativePathUTF8;
				string::GetRelativePath(link1PathUTF8, link1TargetPathUTF8, link1TargetRelativePathUTF8);
				if (link1TargetRelativePathUTF8.empty()) {
					link1TargetPathUTF8 = originalLink1TargetPathUTF8;
				}
				else {
					link1TargetPathUTF8 = link1TargetRelativePathUTF8;
				}
				
				std::string originalLink2TargetPathUTF8(link2TargetPathUTF8);
				if ((link2PathUTF8.find("/Volumes/") == 0) &&
					(link2TargetPathUTF8.find("/Volumes/") == 0)) {
					link2PathUTF8 = link2PathUTF8.substr(8);
					link2TargetPathUTF8 = link2TargetPathUTF8.substr(8);
				}
				
				std::string link2TargetRelativePathUTF8;
				string::GetRelativePath(link2PathUTF8, link2TargetPathUTF8, link2TargetRelativePathUTF8);
				if (link2TargetRelativePathUTF8.empty()) {
					link2TargetPathUTF8 = originalLink2TargetPathUTF8;
				}
				else {
					link2TargetPathUTF8 = link2TargetRelativePathUTF8;
				}
			}
			
			bool match = true;
			if (link1TargetPathUTF8 != link2TargetPathUTF8) {
				match = false;
				FileNotificationParams params(kLinkTargetsDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
						
            bool xattrsMatch = false;
            auto result = CompareXAttrs(h_, inFilePath1, inFilePath2, xattrsMatch);
            if (result != CompareXAttrsResult::kSuccess) {
                NOTIFY_ERROR(h_, "CompareXAttrs failed for:", inFilePath1);
                return kCompareLinksStatus_Error;
            }
            if (!xattrsMatch) {
                match = false;
            }

			uint32_t permissions1 = 0;
			if (!GetFilePosixPermissions(h_, inFilePath1, permissions1)) {
				NOTIFY_ERROR(h_, "CompareLinks: GetFilePosixPermissions failed for path 1:", inFilePath1);
				return kCompareLinksStatus_Error;
			}
			uint32_t permissions2 = 0;
			if (!GetFilePosixPermissions(h_, inFilePath2, permissions2)) {
				NOTIFY_ERROR(h_, "CompareLinks: GetFilePosixPermissions failed for path 2:", inFilePath2);
				return kCompareLinksStatus_Error;
			}
			if (permissions1 != permissions2) {
				match = false;
				FileNotificationParams params(kPermissionsDiffer, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			std::string userOwner1;
			std::string groupOwner1;
			if (!GetFilePosixOwnership(h_, inFilePath1, userOwner1, groupOwner1)) {
				NOTIFY_ERROR(h_, "CompareLinks: GetFilePosixOwnership failed for path 1:", inFilePath1);
				return kCompareLinksStatus_Error;
			}
			
			std::string userOwner2;
			std::string groupOwner2;
			if (!GetFilePosixOwnership(h_, inFilePath1, userOwner2, groupOwner2)) {
				NOTIFY_ERROR(h_, "CompareLinks: GetFilePosixOwnership failed for path 2:", inFilePath2);
				return kCompareLinksStatus_Error;
			}
			
			if (userOwner1 != userOwner2) {
				match = false;
				FileNotificationParams params(kUserOwnersDiffer, inFilePath1, inFilePath2, userOwner1, userOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			if (groupOwner1 != groupOwner2) {
				match = false;
				FileNotificationParams params(kGroupOwnersDiffer, inFilePath1, inFilePath2, groupOwner1, groupOwner2);
				NOTIFY(h_, kFilesDifferNotification, &params);
			}
			
			if (!inIgnoreDates) {
				GetFileDatesCallbackClassT<std::string> datesCallback1;
				GetFileDates(h_, inFilePath1, datesCallback1);
				if (!datesCallback1.mSuccess) {
					NOTIFY_ERROR(h_, "CompareLinks: GetFileDates failed for: ", inFilePath1);
					return kCompareLinksStatus_Error;
				}
				
				GetFileDatesCallbackClassT<std::string> datesCallback2;
				GetFileDates(h_, inFilePath2, datesCallback2);
				if (!datesCallback2.mSuccess) {
					NOTIFY_ERROR(h_, "CompareLinks: GetFileDates failed for: ", inFilePath2);
					return kCompareLinksStatus_Error;
				}
				
				if (datesCallback1.mCreationDate != datesCallback2.mCreationDate) {
					match = false;
					FileNotificationParams params(kCreationDatesDiffer,
												  inFilePath1,
												  inFilePath2,
												  datesCallback1.mCreationDate,
												  datesCallback2.mCreationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
				
				if (datesCallback1.mModificationDate != datesCallback2.mModificationDate) {
					match = false;
					FileNotificationParams params(kModificationDatesDiffer,
												  inFilePath1,
												  inFilePath2,
												  datesCallback1.mModificationDate,
												  datesCallback2.mModificationDate);
					NOTIFY(h_, kFilesDifferNotification, &params);
				}
			}
			
			if (match) {
				FileNotificationParams params(kFilesMatch, inFilePath1, inFilePath2);
				NOTIFY(h_, kFilesMatchNotification, &params);
			}
			
			return kCompareLinksStatus_Success;
		}
		
	} // namespace file
} // namespace hermit
