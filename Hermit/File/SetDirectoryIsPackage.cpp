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
		
		namespace {
			
			//
			class GetFileXAttrsCallbackClass : public GetFileXAttrsCallback {
			public:
				//
				GetFileXAttrsCallbackClass() :
				mResult(kGetFileXAttrsResult_Unknown) {
				}
				
				//
				bool Function(const GetFileXAttrsResult& inResult,
							  const std::string& inOneXAttrName,
							  const std::string& inOneXAttrData) {
					mResult = inResult;
					if (inResult == kGetFileXAttrsResult_Success) {
						if (inOneXAttrName == "com.apple.FinderInfo") {
							mFinderInfo.assign(inOneXAttrData);
							return false;
						}
					}
					return true;
				}
				
				//
				GetFileXAttrsResult mResult;
				std::string mFinderInfo;
			};
			
		} // private namespace
		
		//
		bool SetDirectoryIsPackage(const HermitPtr& h_, const FilePathPtr& inFilePath, const bool& inIsPackage) {
			bool isDirectory = false;
			auto status = PathIsDirectory(h_, inFilePath, isDirectory);
			if (status != PathIsDirectoryStatus::kSuccess) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: PathIsDirectory failed for:", inFilePath);
				return false;
			}
			if (!isDirectory) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: path is not a directory:", inFilePath);
				return false;
			}
			
			GetFileXAttrsCallbackClass xattrs;
			GetFileXAttrs(h_, inFilePath, xattrs);
			if (xattrs.mResult != kGetFileXAttrsResult_Success) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: GetFileXAttrs failed for:", inFilePath);
				return false;
			}
			if (xattrs.mFinderInfo.empty()) {
				// no finder info there yet, we'll make some
				xattrs.mFinderInfo.resize(32);
			}
			else if (xattrs.mFinderInfo.size() != 32) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: mFinderInfo has unexpected size:", xattrs.mFinderInfo.size());
				return false;
			}
			
			FolderInfo* folder2Info = (FolderInfo*)&xattrs.mFinderInfo.at(0);
			auto nsFlags = htons(folder2Info->finderFlags);
			nsFlags |= kHasBundle;
			folder2Info->finderFlags = ntohs(nsFlags);
			
			if (!SetFileXAttr(h_, inFilePath, "com.apple.FinderInfo", xattrs.mFinderInfo)) {
				NOTIFY_ERROR(h_, "SetDirectoryIsPackage: SetFileXAttr failed for:", inFilePath);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit


