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

#ifndef S3DataStore_h
#define S3DataStore_h

#include <memory>
#include "Hermit/DataStore/DataStore.h"
#include "Hermit/S3Bucket/S3Bucket.h"

namespace hermit {
	namespace s3datastore {
		
		//
		class S3DataStore : public datastore::DataStore, public std::enable_shared_from_this<S3DataStore> {
		public:
			//
			S3DataStore(const s3bucket::S3BucketPtr& bucket, bool useReducedRedundancyStorage);
			
			//
			virtual void ListItems(const HermitPtr& h_,
								   const datastore::DataPathPtr& rootPath,
								   const datastore::ListDataStoreItemsItemCallbackPtr& itemCallback,
								   const datastore::ListDataStoreItemsCompletionPtr& completion) override;
			
			//
			virtual void ItemExists(const HermitPtr& h_,
									const datastore::DataPathPtr& itemPath,
									const datastore::ItemExistsInDataStoreCompletionPtr& completion) override;
						
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
			
			//
			s3bucket::S3BucketPtr mBucket;
			bool mUseReducedRedundancyStorage;
		};
		
		//
		typedef std::shared_ptr<S3DataStore> S3DataStorePtr;
		
	} // namespace s3datastore
} // namespace hermit

#endif 
