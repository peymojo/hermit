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

#include "Hermit/Foundation/Notification.h"
#include "DataStore.h"

namespace hermit {
	namespace datastore {
		
		//
		ListDataStoreContentsResult DataStore::ListContents(const HermitPtr& h_,
															const DataPathPtr& inRootPath,
															const ListDataStoreContentsItemCallbackRef& inItemCallback) {
			NOTIFY_ERROR(h_, "unimplemented");
			return ListDataStoreContentsResult::kError;
		}
		
		//
		void DataStore::ItemExists(const HermitPtr& h_,
								   const DataPathPtr& inItemPath,
								   const ItemExistsInDataStoreCallbackRef& inCallback) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		CreateDataStoreLocationIfNeededStatus DataStore::CreateLocationIfNeeded(const HermitPtr& h_, const DataPathPtr& inPath) {
			NOTIFY_ERROR(h_, "unimplemented");
			return kCreateDataStoreLocationIfNeededStatus_Error;
		}
		
		//
		void DataStore::LoadData(const HermitPtr& h_,
								 const DataPathPtr& inPath,
								 const EncryptionSetting& inEncryptionSetting,
								 const LoadDataStoreDataDataBlockPtr& inDataBlock,
								 const LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void DataStore::WriteData(const HermitPtr& h_,
								  const DataPathPtr& inPath,
								  const SharedBufferPtr& inData,
								  const EncryptionSetting& inEncryptionSetting,
								  const WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		bool DataStore::DeleteItem(const HermitPtr& h_, const DataPathPtr& inPath) {
			NOTIFY_ERROR(h_, "unimplemented");
			return false;
		}
		
	} // namespace datastore
} // namespace hermit
