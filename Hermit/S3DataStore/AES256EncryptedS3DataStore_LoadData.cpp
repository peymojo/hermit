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
#include "AES256EncryptedS3DataStore.h"

namespace hermit {
	namespace s3datastore {
        namespace AES256EncryptedS3DataStore_LoadData_Impl {
			
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
				virtual void Call(const HermitPtr& h_, const datastore::LoadDataStoreDataResult& status) override {
					if (status == datastore::LoadDataStoreDataResult::kCanceled) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kCanceled);
						return;
					}
					if (status == datastore::LoadDataStoreDataResult::kItemNotFound) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kItemNotFound);
						return;
					}
					if (status != datastore::LoadDataStoreDataResult::kSuccess) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
						return;
					}
					
					if (mEncryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
						mDataBlock->Call(h_, DataBuffer(mData->mData.data(), mData->mData.size()));
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kSuccess);
						return;
					}
					
					if (mData->mData.size() < 16) {
						NOTIFY_ERROR(h_, "LoadAES256EncryptedFileDataStoreData: dataSize < 16 for item at path:", mPath);
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
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
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
						return;
					}
					mDataBlock->Call(h_, DataBuffer(callback.mData.data(), callback.mData.size()));
					mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kSuccess);
				}
				
				//
				datastore::DataPathPtr mPath;
				datastore::EncryptionSetting mEncryptionSetting;
				std::string mAESKey;
				datastore::LoadDataStoreDataDataPtr mData;
				datastore::LoadDataStoreDataDataBlockPtr mDataBlock;
				datastore::LoadDataStoreDataCompletionBlockPtr mCompletion;
			};
			
		} // namespace AES256EncryptedS3DataStore_LoadData_Impl
        using namespace AES256EncryptedS3DataStore_LoadData_Impl;
		
		//
        void AES256EncryptedS3DataStore::LoadData(const HermitPtr& h_,
                                                  const datastore::DataPathPtr& path,
                                                  const datastore::EncryptionSetting& encryptionSetting,
                                                  const datastore::LoadDataStoreDataDataBlockPtr& dataBlock,
                                                  const datastore::LoadDataStoreDataCompletionBlockPtr& completion) {
			auto loadData = std::make_shared<datastore::LoadDataStoreDataData>();
			auto loadCompletion = std::make_shared<CompletionBlock>(path,
                                                                    encryptionSetting,
                                                                    mAESKey,
                                                                    loadData,
                                                                    dataBlock,
                                                                    completion);
            S3DataStore::LoadData(h_, path, encryptionSetting, loadData, loadCompletion);
		}
		
	} // namespace s3datastore
} // namespace hermit
