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

#ifndef SetFileIsDevice_h
#define SetFileIsDevice_h

#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum SetFileIsDeviceResult {
			kSetFileIsDeviceResult_Unknown,
			kSetFileIsDeviceResult_Success,
			kSetFileIsDeviceResult_PermissionDenied,
			kSetFileIsDeviceResult_Error
		};
		
		//
		SetFileIsDeviceResult SetFileIsDevice(const HermitPtr& h_,
											  const FilePathPtr& inFilePath,
											  const int32_t& inDeviceMode,
											  const int32_t& inDeviceID);
		
	} // namespace file
} // namespace hermit

#endif
