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
#include <sys/xattr.h>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "FilePathToCocoaPathString.h"
#include "GetFileXAttrs.h"

namespace hermit {
	namespace file {
		
		//
		//
		void GetFileXAttrs(const HermitPtr& h_,
						   const FilePathPtr& inFilePath,
						   const GetFileXAttrsCallbackRef& inCallback) {
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, inFilePath, pathUTF8);
			
			ssize_t namesBufActualSize = listxattr(pathUTF8.c_str(), nullptr, 0, XATTR_NOFOLLOW);
			if (namesBufActualSize == -1)
			{
				if (errno == EACCES)
				{
					inCallback.Call(kGetFileXAttrsResult_AccessDenied, "", "");
					return;
				}
				
				NOTIFY_ERROR(h_, "GetFileXAttrs: listxattr (1) failed for path:", inFilePath, "errno:", errno);
				inCallback.Call(kGetFileXAttrsResult_Error, "", "");
				return;
			}
			if (namesBufActualSize == 0)
			{
				inCallback.Call(kGetFileXAttrsResult_Success, "", "");
				return;
			}
			
			std::vector<char> attrNamesBuf(namesBufActualSize);
			namesBufActualSize = listxattr(pathUTF8.c_str(), &attrNamesBuf.at(0), attrNamesBuf.size(), XATTR_NOFOLLOW);
			if (namesBufActualSize == -1)
			{
				NOTIFY_ERROR(h_, "GetFileXAttrs: listxattr (2) failed for path:", inFilePath, "errno:", errno);
				inCallback.Call(kGetFileXAttrsResult_Error, "", "");
				return;
			}
			
			const char* p = &attrNamesBuf.at(0);
			ssize_t offset = 0;
			while (offset < namesBufActualSize)
			{
				ssize_t end = strlen(p);
				std::string oneXAttrName(p, end);
				
				ssize_t valueActualSize = getxattr(
												   pathUTF8.c_str(),
												   oneXAttrName.c_str(),
												   nullptr,
												   0,
												   0,
												   XATTR_NOFOLLOW);
				
				if (valueActualSize == -1)
				{
					NOTIFY_ERROR(h_, "GetFileXAttrs: getxattr (1) failed for path:", inFilePath);
					NOTIFY_ERROR(h_, "-- xattr name:", pathUTF8);
					NOTIFY_ERROR(h_, "-- errno:", errno);
					inCallback.Call(kGetFileXAttrsResult_Error, "", "");
					return;
				}
				
				std::string value;
				if (valueActualSize > 0)
				{
					value.resize(valueActualSize);
					valueActualSize = getxattr(
											   pathUTF8.c_str(),
											   oneXAttrName.c_str(),
											   &value.at(0),
											   value.size(),
											   0,
											   XATTR_NOFOLLOW);
					
					if (valueActualSize == -1)
					{
						NOTIFY_ERROR(h_, "GetFileXAttrs: getxattr (2) failed for path:", inFilePath);
						NOTIFY_ERROR(h_, "-- xattr name:", pathUTF8);
						NOTIFY_ERROR(h_, "-- errno:", errno);
						inCallback.Call(kGetFileXAttrsResult_Error, "", "");
						return;
					}
				}
				if (!inCallback.Call(kGetFileXAttrsResult_Success, oneXAttrName, value))
				{
					break;
				}
				
				offset += (end + 1);
				p += (end + 1);
			}
		}
		
	} // namespace file
} // namespace hermit
