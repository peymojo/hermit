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

#include "CreateS3DataStoreLocationIfNeeded.h"
#include "DeleteS3DataStoreItem.h"
#include "ItemExistsInS3DataStore.h"
#include "ListS3DataStoreContents.h"
#include "LoadS3DataStoreData.h"
#include "S3DataStore.h"
#include "WriteS3DataStoreData.h"

namespace hermit {
	namespace s3datastore {
		
		//
		S3DataStore::S3DataStore(const s3bucket::S3BucketPtr& inBucket, bool inUseReducedRedundancyStorage) :
		mBucket(inBucket),
		mUseReducedRedundancyStorage(inUseReducedRedundancyStorage) {
		}
		
		//
		datastore::ListDataStoreContentsResult
		S3DataStore::ListContents(const HermitPtr& h_,
								  const datastore::DataPathPtr& inRootPath,
								  const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) {
			return ListS3DataStoreContents(h_, shared_from_this(), inRootPath, inItemCallback);
		}
		
		//
		void S3DataStore::ItemExists(const HermitPtr& h_,
									 const datastore::DataPathPtr& inItemPath,
									 const datastore::ItemExistsInDataStoreCallbackRef& inCallback) {
			ItemExistsInS3DataStore(h_, shared_from_this(), inItemPath, inCallback);
		}
		
		
		//
		datastore::CreateDataStoreLocationIfNeededStatus
		S3DataStore::CreateLocationIfNeeded(const HermitPtr& h_, const datastore::DataPathPtr& inPath) {
			return CreateS3DataStoreLocationIfNeeded(h_, shared_from_this(), inPath);
		}
		
		//
		void S3DataStore::LoadData(const HermitPtr& h_,
								   const datastore::DataPathPtr& inPath,
								   const datastore::EncryptionSetting& inEncryptionSetting,
								   const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
								   const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			LoadS3DataStoreData(h_, shared_from_this(), inPath, inEncryptionSetting, inDataBlock, inCompletion);
		}
		
		//
		void S3DataStore::WriteData(const HermitPtr& h_,
									const datastore::DataPathPtr& inPath,
									const SharedBufferPtr& inData,
									const datastore::EncryptionSetting& inEncryptionSetting,
									const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
			WriteS3DataStoreData(h_,
								 shared_from_this(),
								 inPath,
								 inData,
								 inEncryptionSetting,
								 inCompletionFunction);
		}
		
		//
		bool S3DataStore::DeleteItem(const HermitPtr& h_, const datastore::DataPathPtr& inPath) {
			return DeleteS3DataStoreItem(h_, shared_from_this(), inPath);
		}
		
	} // namespace s3datastore
} // namespace hermit
