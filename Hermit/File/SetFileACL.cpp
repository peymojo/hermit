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
#include <sys/types.h>
#include <sys/acl.h>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "SetFileACL.h"

namespace hermit {
	namespace file {
		
		//
		bool SetFileACL(const HermitPtr& h_, const FilePathPtr& inFilePath, const std::string& inACL) {
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			
			acl_t acl = acl_from_text(inACL.c_str());
			if (acl == 0) {
				NOTIFY_ERROR(h_, "SetFileACL: acl_from_text failed for acl text:", inACL);
				NOTIFY_ERROR(h_, "-- errno:", errno);
				return false;
			}
			
			int result = acl_set_link_np(pathUTF8.c_str(), ACL_TYPE_EXTENDED, acl);
			acl_free(acl);
			
			if (result != 0) {
				NOTIFY_ERROR(h_, "SetFileACL: acl_set_link_np failed for path:", inFilePath);
				NOTIFY_ERROR(h_, "-- acl text:", inACL);
				NOTIFY_ERROR(h_, "-- errno:", errno);
				return false;
			}
			return true;
		}
		
	} // namespace file
} // namespace hermit
