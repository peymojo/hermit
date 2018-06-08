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
		void DataStore::ListItems(const HermitPtr& h_,
								  const DataPathPtr& rootPath,
								  const ListDataStoreItemsItemCallbackPtr& itemCallback,
								  const ListDataStoreItemsCompletionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
            completion->Call(h_, ListDataStoreItemsResult::kError);
		}
		
		//
		void DataStore::ItemExists(const HermitPtr& h_,
								   const DataPathPtr& itemPath,
								   const ItemExistsInDataStoreCompletionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
            completion->Call(h_, ItemExistsInDataStoreResult::kError, false);
		}
				
		//
		void DataStore::LoadData(const HermitPtr& h_,
								 const DataPathPtr& path,
								 const EncryptionSetting& encryptionSetting,
								 const LoadDataStoreDataDataBlockPtr& dataBlock,
								 const LoadDataStoreDataCompletionBlockPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
            completion->Call(h_, LoadDataStoreDataResult::kError);
		}
		
		//
		void DataStore::WriteData(const HermitPtr& h_,
								  const DataPathPtr& path,
								  const SharedBufferPtr& data,
								  const EncryptionSetting& encryptionSetting,
								  const WriteDataStoreDataCompletionFunctionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
            completion->Call(h_, WriteDataStoreDataResult::kError);
		}
		
		//
		void DataStore::DeleteItem(const HermitPtr& h_,
                                   const DataPathPtr& path,
                                   const DeleteDataStoreItemCompletionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
            completion->Call(h_, DeleteDataStoreItemResult::kError);
		}
		
	} // namespace datastore
} // namespace hermit
