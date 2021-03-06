//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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

#include "Hermit/Foundation/Notification.h"
#include "GetFileXAttrs.h"
#include "SetFileXAttr.h"
#include "CopyXAttrs.h"

namespace hermit {
	namespace file {
		
		//
		CopyXAttrsResult CopyXAttrs(const HermitPtr& h_, const FilePathPtr& source, const FilePathPtr& dest) {
			GetFileXAttrsCallbackClass xattrs;
			auto result = GetFileXAttrs(h_, source, xattrs);
			if (result != GetFileXAttrsResult::kSuccess) {
				NOTIFY_ERROR(h_, "GetFileXAttrs failed for: ", source);
				return CopyXAttrsResult::kError;
			}
			for (auto it = begin(xattrs.mXAttrs); it != end(xattrs.mXAttrs); ++it) {
				if (!SetFileXAttr(h_, dest, (*it).first, (*it).second)) {
					NOTIFY_ERROR(h_, "SetFileXAttr failed for: ", dest, "attr:", (*it).first);
					return CopyXAttrsResult::kError;
				}
			}
			return CopyXAttrsResult::kSuccess;
		}
		
	} // namespace file
} // namespace hermit
