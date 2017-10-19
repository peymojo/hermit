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

#include <sys/errno.h>
#include <string>
#include <sys/stat.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "SetFileBSDFlags.h"

namespace hermit {
	namespace file {
		
		//
		bool SetFileBSDFlags(const HermitPtr& h_, const FilePathPtr& inFilePath, const uint32_t& inFlags) {
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			int result = lchflags(pathUTF8.c_str(), inFlags);
			if (result != 0) {
				NOTIFY_ERROR(h_, "SetFileBSDFlsgs: lchflags failed, errno:", errno);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit
