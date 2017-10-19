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

#include <errno.h>
#include <string>
#include <sys/stat.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "GetFileBSDFlags.h"

namespace hermit {
	namespace file {
		
		//
		//
		void GetFileBSDFlags(const HermitPtr& h_,
							 const FilePathPtr& inFilePath,
							 const GetFileBSDFlagsCallbackRef& inCallback)
		{
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			
			struct stat fileStat = { 0 };
			int result = lstat(pathUTF8.c_str(), &fileStat);
			if (result != 0) {
				NOTIFY_ERROR(h_, "GetFileBSDFlags: lstat failed for path:", inFilePath);
				NOTIFY_ERROR(h_, "-- errno:", errno);
				inCallback.Call(false, 0);
				return;
			}
			inCallback.Call(true, fileStat.st_flags);
		}
		
	} // namespace file
} // namespace hermit
