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

#include <Carbon/Carbon.h>
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "GetFileXAttrs.h"
#include "PathIsDirectory.h"
#include "SetDirectoryIsPackage.h"
#include "SetFileXAttr.h"

namespace hermit {
	namespace file {
		namespace SetDirectoryIsPackage_Impl {
			
			//
			class GetFinderInfoCallbackClass : public GetFileXAttrsCallback {
			public:
				//
				GetFinderInfoCallbackClass() {
				}
				
				//
				bool Function(const HermitPtr& h_, const std::string& xAttrName, const std::string& xAttrData) {
                    if (xAttrName == "com.apple.FinderInfo") {
                        mFinderInfo.assign(xAttrData);
                        return false;
					}
					return true;
				}
				
				//
				std::string mFinderInfo;
			};
			
		} // namespace SetDirectoryIsPackage_Impl
        using namespace SetDirectoryIsPackage_Impl;
		
		//
		bool SetDirectoryIsPackage(const HermitPtr& h_, const FilePathPtr& filePath, const bool& isPackage) {
			bool isDirectory = false;
			auto status = PathIsDirectory(h_, filePath, isDirectory);
			if (status != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: PathIsDirectory failed for:", filePath);
				return false;
			}
			if (!isDirectory) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: path is not a directory:", filePath);
				return false;
			}
			
			GetFinderInfoCallbackClass info;
			auto result = GetFileXAttrs(h_, filePath, info);
            if (result != GetFileXAttrsResult::kSuccess) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: GetFileXAttrs failed for:", filePath);
				return false;
			}
			if (info.mFinderInfo.empty()) {
				// no finder info there yet, we'll make some
				info.mFinderInfo.resize(32);
			}
			else if (info.mFinderInfo.size() != 32) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: mFinderInfo has unexpected size:", info.mFinderInfo.size());
				return false;
			}
			
			FolderInfo* folder2Info = (FolderInfo*)&info.mFinderInfo.at(0);
			auto nsFlags = htons(folder2Info->finderFlags);
			nsFlags |= kHasBundle;
			folder2Info->finderFlags = ntohs(nsFlags);
			
			if (!SetFileXAttr(h_, filePath, "com.apple.FinderInfo", info.mFinderInfo)) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: SetFileXAttr failed for:", filePath);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit


