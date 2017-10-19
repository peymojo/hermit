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
#include "GetFileACL.h"

namespace hermit {
	namespace file {
		
		//
		void GetFileACL(const HermitPtr& h_,
						const FilePathPtr& inFilePath,
						const GetFileACLCallbackRef& inCallback) {
			
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			
			acl_t acl = acl_get_link_np(pathUTF8.c_str(), ACL_TYPE_EXTENDED);
			if (acl == 0) {
				int err = errno;
				if (err == ENOENT) {
					inCallback.Call(true, "");
					return;
				}
				NOTIFY_ERROR(h_, "GetFileACL: acl_get_link_np failed for path:", inFilePath, "errno:", err);
				inCallback.Call(false, "");
				return;
			}
			
			ssize_t len = 0;
			const char* textP = acl_to_text(acl, &len);
			if (textP == nullptr) {
				NOTIFY_ERROR(h_, "GetFileACL: acl_to_text failed for path:", inFilePath, "errno:", errno);
				inCallback.Call(false, "");
				acl_free(acl);
				return;
			}
			inCallback.Call(true, textP);
			acl_free((void*)textP);
			acl_free(acl);
		}
		
	} // namespace file
} // namespace hermit
