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

#include "AES256EncryptedS3DataStore.h"
#include "LoadAES256EncryptedS3DataStoreData.h"
#include "WriteAES256EncryptedS3DataStoreData.h"

namespace hermit {
	namespace s3datastore {
		
		//
		AES256EncryptedS3DataStore::AES256EncryptedS3DataStore(s3bucket::S3BucketPtr inBucket,
															   bool inUseReducedRedundancyStorage,
															   const std::string& inAESKey) :
		S3DataStore(inBucket, inUseReducedRedundancyStorage),
		mAESKey(inAESKey) {
		}
		
		//
		void AES256EncryptedS3DataStore::LoadData(const HermitPtr& h_,
												  const datastore::DataPathPtr& inPath,
												  const datastore::EncryptionSetting& inEncryptionSetting,
												  const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
												  const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			LoadAES256EncryptedS3DataStoreData(h_, shared_from_this(), inPath, inEncryptionSetting, inDataBlock, inCompletion);
		}
		
		//
		void AES256EncryptedS3DataStore::WriteData(const HermitPtr& h_,
												   const datastore::DataPathPtr& inPath,
												   const SharedBufferPtr& inData,
												   const datastore::EncryptionSetting& inEncryptionSetting,
												   const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
			WriteAES256EncryptedS3DataStoreData(h_,
												shared_from_this(),
												inPath,
												inData,
												inEncryptionSetting,
												inCompletionFunction);
		}
		
	} // namespace s3datastore
} // namespace hermit
