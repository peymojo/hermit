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
#include "FilePathToCocoaPathString.h"
#include "GetFileFinderInfo.h"

namespace hermit {
	namespace file {
		
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
		
		//
		void GetFileFinderInfo(const HermitPtr& h_,
							   const FilePathPtr& inFilePath,
							   const GetFileFinderInfoCallbackRef& inCallback) {
			
			StringCallbackClass stringCallback;
			FilePathToCocoaPathString(inFilePath, h_, stringCallback);
			std::string filePathUTF8(stringCallback.mValue);
			
			FSRef fileRef;
			OSErr err = FSPathMakeRef((const UInt8*)filePathUTF8.c_str(), &fileRef, NULL);
			if (err != noErr) {
				NOTIFY_ERROR(h_,
							 "GetFileFinderInfo: FSPathMakeRef failed for source path:",
							 inFilePath,
							 ", err:",
							 err);
				
				inCallback.Call(false, NULL, 0, NULL, 0);
				return;
			}
			
			FSCatalogInfo fileInfo;
			err = FSGetCatalogInfo(&fileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &fileInfo, NULL, NULL, NULL);
			if (err != noErr) {
				NOTIFY_ERROR(h_,
							 "GetFileFinderInfo: FSGetCatalogInfo failed for source path:",
							 inFilePath,
							 ", err:",
							 err);
				
				inCallback.Call(false, NULL, 0, NULL, 0);
				return;
			}
			
			inCallback.Call(
							true,
							(const char*)fileInfo.finderInfo,
							16,
							(const char*)fileInfo.extFinderInfo,
							16);
		}
		
#pragma GCC diagnostic pop
		
	} // namespace file
} // namespace hermit


