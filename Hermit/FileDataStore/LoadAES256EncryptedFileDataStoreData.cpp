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

#include <string>
#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Encoding/AES256DecryptCBC.h"
#include "Hermit/Foundation/Notification.h"
#include "AES256EncryptedFileDataStore.h"
#include "LoadFileDataStoreData.h"
#include "LoadAES256EncryptedFileDataStoreData.h"

namespace hermit {
	namespace filedatastore {
		
		namespace {
			
			//
			class CompletionBlock : public datastore::LoadDataStoreDataCompletionBlock {
			public:
				//
				CompletionBlock(const datastore::DataPathPtr& inPath,
								const datastore::EncryptionSetting& inEncryptionSetting,
								const std::string& inAESKey,
								const datastore::LoadDataStoreDataDataPtr& inData,
								const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
								const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) :
				mPath(inPath),
				mEncryptionSetting(inEncryptionSetting),
				mAESKey(inAESKey),
				mData(inData),
				mDataBlock(inDataBlock),
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const datastore::LoadDataStoreDataStatus& status) override {
					if (status == datastore::LoadDataStoreDataStatus::kCanceled) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kCanceled);
						return;
					}
					if (status == datastore::LoadDataStoreDataStatus::kItemNotFound) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kItemNotFound);
						return;
					}
					if (status != datastore::LoadDataStoreDataStatus::kSuccess) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kError);
						return;
					}
					
					if (mEncryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
						mDataBlock->Call(DataBuffer(mData->mData.data(), mData->mData.size()));
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kSuccess);
						return;
					}
					
					if (mData->mData.size() < 16) {
						NOTIFY_ERROR(h_, "LoadAES256EncryptedFileDataStoreData: dataSize < 16 for item at path:", mPath);
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kError);
						return;
					}
					
					const char* p = mData->mData.data();
					size_t size = mData->mData.size();
					
					std::string inputVector(p, 16);
					p += 16;
					size -= 16;
					
					encoding::AES256DecryptCBCCallbackClass callback;
					encoding::AES256DecryptCBC(h_, DataBuffer(p, size), mAESKey, inputVector, callback);
					if (!callback.mSuccess) {
						NOTIFY_ERROR(h_, "LoadAES256EncryptedFileDataStoreData: AES256DecryptCBC failed for item at path:", mPath);
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kError);
						return;
					}
					mDataBlock->Call(DataBuffer(callback.mData.data(), callback.mData.size()));
					mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kSuccess);
				}

				//
				datastore::DataPathPtr mPath;
				datastore::EncryptionSetting mEncryptionSetting;
				std::string mAESKey;
				datastore::LoadDataStoreDataDataPtr mData;
				datastore::LoadDataStoreDataDataBlockPtr mDataBlock;
				datastore::LoadDataStoreDataCompletionBlockPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void LoadAES256EncryptedFileDataStoreData(const HermitPtr& h_,
												  const datastore::DataStorePtr& inDataStore,
												  const datastore::DataPathPtr& inPath,
												  const datastore::EncryptionSetting& inEncryptionSetting,
												  const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
												  const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			AES256EncryptedFileDataStore& dataStore = static_cast<AES256EncryptedFileDataStore&>(*inDataStore);
			auto dataBlock = std::make_shared<datastore::LoadDataStoreDataData>();
			auto completion = std::make_shared<CompletionBlock>(inPath,
																inEncryptionSetting,
																dataStore.mAESKey,
																dataBlock,
																inDataBlock,
																inCompletion);
			LoadFileDataStoreData(h_, inDataStore, inPath, inEncryptionSetting, dataBlock, completion);
		}
		
	} // namespace filedatastore
} // namespace hermit
