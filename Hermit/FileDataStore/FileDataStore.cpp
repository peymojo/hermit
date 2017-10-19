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

#include "CreateFileDataStoreLocationIfNeeded.h"
#include "DeleteFileDataStoreItem.h"
#include "FileDataStore.h"
#include "ItemExistsInFileDataStore.h"
#include "ListFileDataStoreContents.h"
#include "LoadFileDataStoreData.h"
#include "WriteFileDataStoreData.h"

namespace hermit {
	namespace filedatastore {

		//
		FileDataStore::FileDataStore() {
		}
		
		//
		datastore::ListDataStoreContentsResult
		FileDataStore::ListContents(const HermitPtr& h_,
									const datastore::DataPathPtr& inRootPath,
									const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) {
			return ListFileDataStoreContents(h_, shared_from_this(), inRootPath, inItemCallback);
		}
		
		//
		void FileDataStore::ItemExists(const HermitPtr& h_,
									   const datastore::DataPathPtr& inItemPath,
									   const datastore::ItemExistsInDataStoreCallbackRef& inCallback) {
			ItemExistsInFileDataStore(h_, shared_from_this(), inItemPath, inCallback);
		}
		
		
		//
		datastore::CreateDataStoreLocationIfNeededStatus
		FileDataStore::CreateLocationIfNeeded(const HermitPtr& h_, const datastore::DataPathPtr& inPath) {
			return CreateFileDataStoreLocationIfNeeded(h_, shared_from_this(), inPath);
		}
		
		//
		void FileDataStore::LoadData(const HermitPtr& h_,
									 const datastore::DataPathPtr& inPath,
									 const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
									 const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
									 const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			LoadFileDataStoreData(h_, shared_from_this(), inPath, inEncryptionSetting, inDataBlock, inCompletion);
		}
		
		//
		void FileDataStore::WriteData(const HermitPtr& h_,
									  const datastore::DataPathPtr& inPath,
									  const SharedBufferPtr& inData,
									  const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
									  const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
			WriteFileDataStoreData(h_,
								   shared_from_this(),
								   inPath,
								   inData,
								   inEncryptionSetting,
								   inCompletionFunction);
		}
		
		//
		bool FileDataStore::DeleteItem(const HermitPtr& h_, const datastore::DataPathPtr& inPath) {
			return DeleteFileDataStoreItem(h_, shared_from_this(), inPath);
		}

	} // namespace filedatastore
} // namespace hermit
