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

#if 000

#include <string>
#include <vector>
#include "Hermit/File/API/GetFileForkNames.h"
#include "Hermit/File/API/GetFileForkSize.h"
#include "Hermit/Log/LogFilePath.h"
#include "Hermit/Log/LogString.h"
#include "GetFileTotalSize.h"

namespace e38
{

//
//
void GetFileTotalSize(
	const ConstHubPtr& inHub,
	const FilePathPtr& inFilePath,
	const GetFileTotalSizeCallbackRef& inCallback)
{
	typedef std::vector<std::string> StringVector;
	GetFileForkNamesCallbackClassT<StringVector> callback;
	GetFileForkNames(inHub, inFilePath, callback);
	if (callback.mForkNames.empty())
	{
		LogFilePath(
			inHub,
			"GetFileTotalSize: GetFileForkNames failed for: ",
			inFilePath);
			
		inCallback.Call(inHub, false, 0);
		return;
	}

	uint64_t totalFileSize = 0;
	std::vector<std::string>::const_iterator end = callback.mForkNames.end();
	for (std::vector<std::string>::const_iterator it = callback.mForkNames.begin(); it != end; ++it)
	{
		std::string forkName(*it);
		
		GetFileForkSizeCallbackClass sizeCallback;
		GetFileForkSize(
			inHub,
			inFilePath,
			forkName,
			sizeCallback);

		if (!sizeCallback.mSuccess)
		{
			//	Some inconsistency in Carbon APIs reporting a file having a fork and then returning
			//	an error on forkSize... [???]

			LogFilePath(
				inHub,
				"GetFileTotalSize: GetFileForkSize failed for: ",
				inFilePath);

			LogString(
				inHub,
				"... fork name: ",
				forkName);

			inCallback.Call(inHub, false, 0);
			return;
		}

		uint64_t forkSize = sizeCallback.mSize;
		totalFileSize += forkSize;
	}
		
	inCallback.Call(inHub, true, totalFileSize);
}

} // namespace e38

#endif
