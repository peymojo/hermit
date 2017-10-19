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
#include <vector>
#include "Hermit/Encoding/CreateUUIDString.h"
#include "Hermit/String/AddTrailingSlash.h"
#include "CreateFilePathFromUTF8String.h"
#include "GetTemporaryFilePath.h"

#if 000

namespace hermit
{

//
//
void GetTemporaryFilePath(
	const ConstHubPtr& inHub,
	const FilePathCallbackRef& inCallback)
{
	FSRef ref;
	OSErr err = FSFindFolder(kUserDomain, kTemporaryFolderType, true, &ref);
	if (err != noErr)
	{
		Log(
			inHub,
			"GetTemporaryFilePath: FSFindFolder failed.");

		LogSInt32(
			inHub,
			" -- err: ",
			err);
		
		inCallback.Call(inHub, 0);
	}
	
	std::vector<UInt8> pathVec(32768);
	OSStatus status = FSRefMakePath(&ref, &pathVec.at(0), 32767);
	if (status != noErr)
	{
		Log(
			inHub,
			"GetTemporaryFilePath: FSRefMakePath failed.");

		LogSInt32(
			inHub,
			" -- status: ",
			status);

		inCallback.Call(inHub, 0);
	}
	
	std::string path((const char*)&pathVec.at(0));

	typedef StringCallbackClass StringCallback;
	StringCallback stringCallback;
	AddTrailingSlash(
		inHub,
		path,
		stringCallback);
	path = stringCallback.mValue;

	StringCallback uuidCallback;
	CreateUUIDString(
		inHub,
		uuidCallback);
	std::string guidStr(uuidCallback.mValue);

	path += guidStr;
	path += ".tmp";
			
	CreateFilePathFromUTF8String(inHub, path, inCallback);
}

} // namespace e38

#endif

