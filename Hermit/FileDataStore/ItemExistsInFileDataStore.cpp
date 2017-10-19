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

#include "Hermit/File/FileExists.h"
#include "Hermit/Foundation/Notification.h"
#include "FilePathDataPath.h"
#include "ItemExistsInFileDataStore.h"

namespace hermit {
	namespace filedatastore {
		
		//
		void ItemExistsInFileDataStore(const HermitPtr& h_,
									   const datastore::DataStorePtr& inDataStore,
									   const datastore::DataPathPtr& inItemPath,
									   const datastore::ItemExistsInDataStoreCallbackRef& inCallback) {
			FilePathDataPath& filePath = static_cast<FilePathDataPath&>(*inItemPath);
			
			file::FileExistsCallbackClass fileExistsCallback;
			file::FileExists(h_, filePath.mFilePath, fileExistsCallback);
			if (!fileExistsCallback.mSuccess) {
				NOTIFY_ERROR(h_, "ItemExistsInFileDataStore: FileExists failed for:", filePath.mFilePath);
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kError, false);
				return;
			}
			
			if (fileExistsCallback.mExists) {
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, true);
			}
			else {
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, false);
			}
		}
		
	} // namespace filedatastore
} // namespace hermit
