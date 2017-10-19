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
#include "GetFilePosixPermissions.h"
#include "SetFilePosixPermissions.h"
#include "CopyFinderInfo.h"

#if 000

namespace hermit {
namespace file {

//
//
void CopyFinderInfo(
	const FilePathPtr& inSourcePath,
	const FilePathPtr& inDestPath,
	const BasicCallbackRef& inCallback)
{
	StringCallbackClass stringCallback;
	FilePathToCocoaPathString(inSourcePath, stringCallback);
	std::string sourcePathUTF8(stringCallback.mValue);

	FSRef sourceFileRef;
	OSErr err = FSPathMakeRef((const UInt8*)sourcePathUTF8.c_str(), &sourceFileRef, NULL);
	if (err != noErr)
	{
		LogFilePath(
			"CopyFinderInfo: FSPathMakeRef failed for source path: ",
			inSourcePath);
			
		LogSInt32(
			"-- err: ",
			err);
		
		inCallback.Call(false);
		return;		
	}

	FSCatalogInfo sourceFileInfo;
	err = FSGetCatalogInfo(&sourceFileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &sourceFileInfo, NULL, NULL, NULL);
	if (err != noErr)
	{
		LogFilePath(
			"CopyFinderInfo: FSGetCatalogInfo failed for source path: ",
			inSourcePath);
			
		LogSInt32(
			"-- err: ",
			err);
		
		inCallback.Call(false);
		return;		
	}

	FilePathToCocoaPathString(inDestPath, stringCallback);
	std::string destPathUTF8(stringCallback.mValue);

	FSRef destFileRef;
	err = FSPathMakeRef((const UInt8*)destPathUTF8.c_str(), &destFileRef, NULL);
	if (err != noErr)
	{
		LogFilePath(
			"CopyFinderInfo: FSPathMakeRef failed for dest path: ",
			inDestPath);
			
		LogSInt32(
			"-- err: ",
			err);
		
		inCallback.Call(false);
		return;		
	}

	FSCatalogInfo destFileInfo;
	err = FSGetCatalogInfo(&destFileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &destFileInfo, NULL, NULL, NULL);
	if (err != noErr)
	{
		LogFilePath(
			"CopyFinderInfo: FSGetCatalogInfo failed for dest path: ",
			inDestPath);
			
		LogSInt32(
			"-- err: ",
			err);
		
		inCallback.Call(false);
		return;		
	}
	
	for (int n = 0; n < 16; ++n)
	{
		destFileInfo.finderInfo[n] = sourceFileInfo.finderInfo[n];
		destFileInfo.extFinderInfo[n] = sourceFileInfo.extFinderInfo[n];
	}

	GetFilePosixPermissionsCallbackClass getPermissionsCallback;
	GetFilePosixPermissions(inDestPath, getPermissionsCallback);
	if (!getPermissionsCallback.mSuccess)
	{
		LogFilePath(
			"CopyFinderInfo: GetFilePosixPermissions failed for dest path: ",
			inDestPath);

		inCallback.Call(false);
		return;		
	}

	uint32_t permissions = getPermissionsCallback.mPermissions | 128;
	BasicCallbackClass setPermissionsCallback;
	SetFilePosixPermissions(inDestPath, permissions, setPermissionsCallback);
	if (!setPermissionsCallback.mSuccess)
	{
		LogFilePath(
			"CopyFinderInfo: SetFilePosixPermissions (1) failed for dest path: ",
			inDestPath);

		inCallback.Call(false);
		return;		
	}

	bool success = true;
	err = FSSetCatalogInfo(&destFileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &destFileInfo);
	if (err != noErr)
	{
		LogFilePath(
			"CopyFinderInfo: FSSetCatalogInfo failed for dest path: ",
			inDestPath);
			
		LogSInt32(
			"-- err: ",
			err);
		
		success = false;
	}

	SetFilePosixPermissions(inDestPath, getPermissionsCallback.mPermissions, setPermissionsCallback);
	if (!setPermissionsCallback.mSuccess)
	{
		LogFilePath(
			"CopyFinderInfo: SetFilePosixPermissions (2) failed for dest path: ",
			inDestPath);

	}
	
	inCallback.Call(success);
}

} // namespace file
} // namespace hermit

#endif

