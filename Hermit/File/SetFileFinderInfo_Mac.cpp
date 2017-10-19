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
#include "SetFileFinderInfo.h"

#if 000

namespace e38
{

#pragma GCC diagnostic push 
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
//
//
void SetFileFinderInfo(
	const ConstHubPtr& inHub,
	const FilePathPtr& inFilePath,
	const std::string& inFinderInfo,
	const uint64_t& inFinderInfoSize,
	const std::string& inFinderExtendedInfo,
	const uint64_t& inFinderExtendedInfoSize,
	const BasicCallbackRef& inCallback)
{
	StringCallbackClass stringCallback;
	FilePathToCocoaPathString(inHub, inFilePath, stringCallback);
	std::string filePathUTF8(stringCallback.mValue);

	FSRef fileRef;
	OSErr err = FSPathMakeRef((const UInt8*)filePathUTF8.c_str(), &fileRef, NULL);
	if (err != noErr)
	{
		LogFilePath(
			inHub,
			"SetFileFinderInfo: FSPathMakeRef failed for path: ",
			inFilePath);
			
		LogSInt32(
			inHub,
			"-- err: ",
			err);
		
		inCallback.Call(inHub, false);
		return;		
	}

	FSCatalogInfo fileInfo;
	err = FSGetCatalogInfo(&fileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &fileInfo, NULL, NULL, NULL);
	if (err != noErr)
	{
		LogFilePath(
			inHub,
			"SetFileFinderInfo: FSGetCatalogInfo failed for path: ",
			inFilePath);
			
		LogSInt32(
			inHub,
			"-- err: ",
			err);
		
		inCallback.Call(inHub, false);
		return;		
	}
	
	uint64_t bytesToCopy = 16;
	if (bytesToCopy > inFinderInfoSize)
	{
		bytesToCopy = inFinderInfoSize;
	}
	for (int n = 0; n < bytesToCopy; ++n)
	{
		fileInfo.finderInfo[n] = inFinderInfo[n];
	}
	bytesToCopy = 16;
	if (bytesToCopy > inFinderExtendedInfoSize)
	{
		bytesToCopy = inFinderExtendedInfoSize;
	}
	for (int n = 0; n < bytesToCopy; ++n)
	{
		fileInfo.extFinderInfo[n] = inFinderExtendedInfo[n];
	}

	bool success = true;
	err = FSSetCatalogInfo(&fileRef, kFSCatInfoFinderInfo | kFSCatInfoFinderXInfo, &fileInfo);
	if (err != noErr)
	{
		LogFilePath(
			inHub,
			"SetFileFinderInfo: FSSetCatalogInfo failed for dest path: ",
			inFilePath);
			
		LogSInt32(
			inHub,
			"-- err: ",
			err);
		
		success = false;
	}
	
	inCallback.Call(inHub, success);
}
#pragma GCC diagnostic pop

} // namespace e38

#endif
