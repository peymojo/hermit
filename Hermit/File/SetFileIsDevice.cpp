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
#include <unistd.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "SetFileIsDevice.h"

namespace hermit {
	namespace file {
		
		//
		SetFileIsDeviceResult SetFileIsDevice(const HermitPtr& h_,
											  const FilePathPtr& inFilePath,
											  const int32_t& inDeviceMode,
											  const int32_t& inDeviceID) {
			
			std::string itemPathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, itemPathUTF8);
			
			mode_t mode = inDeviceMode;
			if (S_ISFIFO(mode)) {
				int result = mkfifo(itemPathUTF8.c_str(), mode);
				if (result != 0) {
					int err = errno;
					if (err == EPERM) {
						NOTIFY_ERROR(h_, "SetFileIsDevice: mkfifo permission denied for:", inFilePath);
						return kSetFileIsDeviceResult_PermissionDenied;
					}
					NOTIFY_ERROR(h_, "SetFileIsDevice: mkfifo failed for file:", inFilePath, "errno:", err);
					return kSetFileIsDeviceResult_Error;
				}
				return kSetFileIsDeviceResult_Success;
			}
			int result = mknod(itemPathUTF8.c_str(), mode, inDeviceID);
			if (result != 0) {
				int err = errno;
				if (err == EPERM) {
					return kSetFileIsDeviceResult_PermissionDenied;
				}
				NOTIFY_ERROR(h_, "SetFileIsDevice: mknod failed for file:", inFilePath, "errno:", err, "mode:", mode);
				return kSetFileIsDeviceResult_Error;
			}
			return kSetFileIsDeviceResult_Success;
		}
		
	} // namespace file
} // namespace hermit
