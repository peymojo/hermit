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

#include "Hermit/File/CreateDirectoryIfNeeded.h"
#include "Hermit/Foundation/Notification.h"
#include "FileDataStore.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		
		//
		void FileDataStore::CreateLocationIfNeeded(const HermitPtr& h_,
                                                   const datastore::DataPathPtr& path,
                                                   const datastore::CreateDataStoreLocationIfNeededCompletionPtr& completion) {
			FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*path);
			if (dataPath.mFilePath == nullptr) {
				NOTIFY_ERROR(h_, "dataPath.mFilePath is null.");
                completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kError);
                return;
			}
			
			auto result = file::CreateDirectoryIfNeeded(h_, dataPath.mFilePath);
			if (result.first == file::kCreateDirectoryIfNeededStatus_Success) {
				completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kSuccess);
                return;
			}
            if (result.first == file::kCreateDirectoryIfNeededStatus_ConflictAtPath) {
                completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kConflictAtPath);
                return;
			}
            if (result.first == file::kCreateDirectoryIfNeededStatus_DiskFull) {
				completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kStorageFull);
                return;
			}

            NOTIFY_ERROR(h_, "CreateDirectoryIfNeeded failed for path:", dataPath.mFilePath);
			completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kError);
		}
		
	} // namespace filedatastore
} // namespace hermit
