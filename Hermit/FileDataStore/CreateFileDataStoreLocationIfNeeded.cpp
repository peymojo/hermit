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
#include "FilePathDataPath.h"
#include "CreateFileDataStoreLocationIfNeeded.h"

namespace hermit {
	namespace filedatastore {
		
		//
		datastore::CreateDataStoreLocationIfNeededStatus
		CreateFileDataStoreLocationIfNeeded(const HermitPtr& h_,
											const datastore::DataStorePtr& inDataStore,
											const datastore::DataPathPtr& inPath) {
			
			FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*inPath);
			if (dataPath.mFilePath == nullptr) {
				NOTIFY_ERROR(h_, "CreateFileDataStoreLocationIfNeeded: dataPath.mFilePath is null.");
				return datastore::kCreateDataStoreLocationIfNeededStatus_Error;
			}
			
			auto result = file::CreateDirectoryIfNeeded(h_, dataPath.mFilePath);
			if (result.first == file::kCreateDirectoryIfNeededStatus_Success) {
				return datastore::kCreateDataStoreLocationIfNeededStatus_Success;
			}
			else if (result.first == file::kCreateDirectoryIfNeededStatus_ConflictAtPath) {
				return datastore::kCreateDataStoreLocationIfNeededStatus_ConflictAtPath;
			}
			else if (result.first == file::kCreateDirectoryIfNeededStatus_DiskFull) {
				return datastore::kCreateDataStoreLocationIfNeededStatus_StorageFull;
			}
			else {
				NOTIFY_ERROR(h_,
							 "CreateFileDataStoreLocationIfNeeded: CreateDirectoryIfNeeded failed for path:",
							 dataPath.mFilePath);
				
				return datastore::kCreateDataStoreLocationIfNeededStatus_Error;
			}
		}
		
	} // namespace filedatastore
} // namespace hermit
