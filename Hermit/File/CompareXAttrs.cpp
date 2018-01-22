//
//    Hermit
//    Copyright (C) 2018 Paul Young (aka peymojo)
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

#include "Hermit/Foundation/Notification.h"
#include "GetFileXAttrs.h"
#include "FileNotification.h"
#include "CompareXAttrs.h"

namespace hermit {
    namespace file {
        
        //
        CompareXAttrsResult CompareXAttrs(const HermitPtr& h_, const FilePathPtr& file1, const FilePathPtr& file2, bool& outMatch) {
            GetFileXAttrsCallbackClass xattrs1;
            auto result1 = GetFileXAttrs(h_, file1, xattrs1);
            if (result1 != GetFileXAttrsResult::kSuccess) {
                NOTIFY_ERROR(h_, "GetFileXAttrs failed for: ", file1);
                FileNotificationParams params(kErrorReadingXAttrs, file1);
                NOTIFY(h_, kFileErrorNotification, &params);
                return CompareXAttrsResult::kError;
            }
            
            GetFileXAttrsCallbackClass xattrs2;
            auto result2 = GetFileXAttrs(h_, file2, xattrs2);
            if (result2 != GetFileXAttrsResult::kSuccess) {
                NOTIFY_ERROR(h_, "GetFileXAttrs failed for: ", file2);
                FileNotificationParams params(kErrorReadingXAttrs, file2);
                NOTIFY(h_, kFileErrorNotification, &params);
                return CompareXAttrsResult::kError;
            }

            bool match = true;
            auto it1 = begin(xattrs1.mXAttrs);
            auto end1 = end(xattrs1.mXAttrs);
            auto it2 = begin(xattrs2.mXAttrs);
            auto end2 = end(xattrs2.mXAttrs);
            while ((it1 != end1) && (it2 != end2)) {
                if (it1->first < it2->first) {
                    match = false;
                    FileNotificationParams params(kXAttrPresenceMismatch, file1, file2, it1->first, "");
                    NOTIFY(h_, kFilesDifferNotification, &params);
                    it1++;
                }
                else if (it1->first > it2->first) {
                    match = false;
                    FileNotificationParams params(kXAttrPresenceMismatch, file1, file2, "", it2->first);
                    NOTIFY(h_, kFilesDifferNotification, &params);
                    it2++;
                }
                else {
                    if (it1->second != it2->second) {
                        match = false;
                        FileNotificationParams params(kXAttrValuesDiffer, file1, file2, it1->first, "");
                        NOTIFY(h_, kFilesDifferNotification, &params);
                    }
                    it1++;
                    it2++;
                }
            }
            while (it1 != end1) {
                match = false;
                FileNotificationParams params(kXAttrPresenceMismatch, file1, file2, it1->first, "");
                NOTIFY(h_, kFilesDifferNotification, &params);
                it1++;
            }
            while (it2 != end2) {
                match = false;
                FileNotificationParams params(kXAttrPresenceMismatch, file1, file2, "", it2->first);
                NOTIFY(h_, kFilesDifferNotification, &params);
                it2++;
            }
            outMatch = match;
            return CompareXAttrsResult::kSuccess;
        }
        
    } // namespace file
} // namespace hermit

