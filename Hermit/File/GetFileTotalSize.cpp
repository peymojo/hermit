//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "GetFileDataSize.h"
#include "GetFileTotalSize.h"

namespace hermit {
    namespace file {
        
        //
        void GetFileTotalSize(const HermitPtr& h_, const FilePathPtr& filePath, const GetFileTotalSizeCallbackRef& callback) {
            std::uint64_t dataSize = 0;
            if (!GetFileDataSize(h_, filePath, dataSize)) {
                NOTIFY_ERROR(h_, "GetFileDataSize failed for:", filePath);
                callback.Call(h_, false, 0);
                return;
            }
            callback.Call(h_, true, dataSize);
        }
        
    } // namespace file
} // namespace hermit

