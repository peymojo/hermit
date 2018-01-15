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

#ifndef FileDataStore_h
#define FileDataStore_h

#include <memory>
#include "Hermit/DataStore/DataStore.h"

namespace hermit {
	namespace filedatastore {

		//
		class FileDataStore : public datastore::DataStore, public std::enable_shared_from_this<FileDataStore> {
		public:
			//
			FileDataStore();

			//
			virtual void ListContents(const HermitPtr& h_,
                                      const datastore::DataPathPtr& rootPath,
                                      const datastore::ListDataStoreContentsItemCallbackPtr& itemCallback,
                                      const datastore::ListDataStoreContentsCompletionPtr& completion) override;
			
			//
			virtual void ItemExists(const HermitPtr& h_,
									const datastore::DataPathPtr& itemPath,
									const datastore::ItemExistsInDataStoreCompletionPtr& completion) override;
			
			
			//
			virtual void CreateLocationIfNeeded(const HermitPtr& h_,
                                                const datastore::DataPathPtr& path,
                                                const datastore::CreateDataStoreLocationIfNeededCompletionPtr& completion) override;
			
			//
			virtual void LoadData(const HermitPtr& h_,
								  const datastore::DataPathPtr& path,
								  const datastore::EncryptionSetting& encryptionSetting,
								  const datastore::LoadDataStoreDataDataBlockPtr& dataBlock,
								  const datastore::LoadDataStoreDataCompletionBlockPtr& completion) override;
			
			//
			virtual void WriteData(const HermitPtr& h_,
								   const datastore::DataPathPtr& path,
								   const SharedBufferPtr& data,
								   const datastore::EncryptionSetting& encryptionSetting,
								   const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) override;
			
			//
			virtual void DeleteItem(const HermitPtr& h_,
                                    const datastore::DataPathPtr& path,
                                    const datastore::DeleteDataStoreItemCompletionPtr& completion) override;
		};
		typedef std::shared_ptr<FileDataStore> FileDataStorePtr;

	} // namespace filedatastore
} // namespace hermit

#endif 
