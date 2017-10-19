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
#include <sys/stat.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "GetFileIsDevice.h"

namespace hermit {
	namespace file {
		
		//
		//
		void GetFileIsDevice(const HermitPtr& h_,
							 const FilePathPtr& inFilePath,
							 const GetFileIsDeviceCallbackRef& inCallback)
		{
			std::string itemPathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, itemPathUTF8);
			
			struct stat s;
			int err = lstat(itemPathUTF8.c_str(), &s);
			if (err != 0)
			{
				NOTIFY_ERROR(h_, "GetFileIsDevice: lstat failed for path:", inFilePath, "err:", err);
				inCallback.Call(false, false, 0, 0);
				return;
			}
			
			if (S_ISREG(s.st_mode) || S_ISDIR(s.st_mode) || S_ISLNK(s.st_mode))
			{
				//	regular file, directory, or symlink
				inCallback.Call(true, false, 0, 0);
				return;
			}
			
			if (S_ISBLK(s.st_mode) || S_ISCHR(s.st_mode))
			{
				//	block or character device
				inCallback.Call(true, true, s.st_mode, s.st_rdev);
				return;
			}
			if (S_ISFIFO(s.st_mode) || S_ISSOCK(s.st_mode))
			{
				//	fifo or socket
				inCallback.Call(true, true, s.st_mode, 0);
				return;
			}
			
			NOTIFY_ERROR(h_, "GetFileIsDevice: unknown device type for path:", inFilePath, "s.st_mode:", s.st_mode);
			inCallback.Call(false, false, 0, 0);
		}
		
	} // namespace file
} // namespace hermit
