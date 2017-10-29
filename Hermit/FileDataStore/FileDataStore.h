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
			virtual datastore::ListDataStoreContentsResult ListContents(const HermitPtr& h_,
																		const datastore::DataPathPtr& inRootPath,
																		const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) override;
			
			//
			virtual void ItemExists(const HermitPtr& h_,
									const datastore::DataPathPtr& inItemPath,
									const datastore::ItemExistsInDataStoreCallbackRef& inCallback) override;
			
			
			//
			virtual datastore::CreateDataStoreLocationIfNeededStatus CreateLocationIfNeeded(const HermitPtr& h_,
																							const datastore::DataPathPtr& inPath) override;
			
			//
			virtual void LoadData(const HermitPtr& h_,
								  const datastore::DataPathPtr& inPath,
								  const datastore::EncryptionSetting& inEncryptionSetting,
								  const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
								  const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) override;
			
			//
			virtual void WriteData(const HermitPtr& h_,
								   const datastore::DataPathPtr& inPath,
								   const SharedBufferPtr& inData,
								   const datastore::EncryptionSetting& inEncryptionSetting,
								   const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual bool DeleteItem(const HermitPtr& h_, const datastore::DataPathPtr& inPath) override;
		};
		
		//
		typedef std::shared_ptr<FileDataStore> FileDataStorePtr;

	} // namespace filedatastore
} // namespace hermit

#endif 
