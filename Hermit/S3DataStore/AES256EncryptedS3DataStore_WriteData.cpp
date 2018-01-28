//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include "Hermit/Encoding/AES256EncryptCBC.h"
#include "Hermit/Encoding/CreateInputVector.h"
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Notification.h"
#include "AES256EncryptedS3DataStore.h"

namespace hermit {
    namespace s3datastore {
        namespace AES256EncryptedS3DataStore_WriteData_Impl {
            
            //
            class Task : public AsyncTask {
            public:
                //
                Task(const AES256EncryptedS3DataStorePtr& dataStore,
                     const datastore::DataPathPtr& path,
                     const SharedBufferPtr& data,
                     const datastore::EncryptionSetting& encryptionSetting,
                     const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) :
                mDataStore(dataStore),
                mPath(path),
                mData(data),
                mEncryptionSetting(encryptionSetting),
                mCompletion(completion) {
                }
                
                //
				virtual void PerformTask(const HermitPtr& h_) override {
                    mDataStore->DoWriteData(h_,
                                            mPath,
                                            mData,
                                            mEncryptionSetting,
                                            mCompletion);
                }
                
                //
                AES256EncryptedS3DataStorePtr mDataStore;
                datastore::DataPathPtr mPath;
                SharedBufferPtr mData;
                datastore::EncryptionSetting mEncryptionSetting;
                datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
            };
            
        } // namespace AES256EncryptedS3DataStore_WriteData_Impl
        using namespace AES256EncryptedS3DataStore_WriteData_Impl;
        
        //
        void AES256EncryptedS3DataStore::WriteData(const HermitPtr& h_,
                                                   const datastore::DataPathPtr& path,
                                                   const SharedBufferPtr& data,
                                                   const datastore::EncryptionSetting& encryptionSetting,
                                                   const datastore::WriteDataStoreDataCompletionFunctionPtr& completionFunction) {
            auto task = std::make_shared<Task>(std::static_pointer_cast<AES256EncryptedS3DataStore>(shared_from_this()),
                                               path,
                                               data,
                                               encryptionSetting,
                                               completionFunction);
            if (!QueueAsyncTask(h_, task, 15)) {
                NOTIFY_ERROR(h_, "QueueAsyncTask failed");
                completionFunction->Call(h_, datastore::WriteDataStoreDataResult::kError);
            }
        }
        
        //
        void AES256EncryptedS3DataStore::DoWriteData(const HermitPtr& h_,
                                                     const datastore::DataPathPtr& path,
                                                     const SharedBufferPtr& data,
                                                     const datastore::EncryptionSetting& encryptionSetting,
                                                     const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
            if (CHECK_FOR_ABORT(h_)) {
                completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
                return;
            }
            
            std::string encryptedS3Data;
            if (encryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
                encryptedS3Data.assign(data->Data(), data->Size());
            }
            else {
                std::string inputVector;
                encoding::CreateInputVector(h_, 16, inputVector);
                encoding::AES256EncryptCBCCallbackClass dataCallback;
                encoding::AES256EncryptCBC(h_,
                                           DataBuffer(data->Data(), data->Size()),
                                           mAESKey,
                                           inputVector,
                                           dataCallback);
                
                if (dataCallback.mStatus == encoding::kAES256EncryptCBC_Canceled) {
                    completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
                    return;
                }
                if (dataCallback.mStatus != encoding::kAES256EncryptCBC_Success) {
                    NOTIFY_ERROR(h_, "WriteAES256EncryptedS3DataStoreData: AES256EncryptCBC failed.");
                    completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
                    return;
                }
                
                encryptedS3Data = inputVector;
                encryptedS3Data += dataCallback.mValue;
            }
            
            SharedBufferPtr buffer = std::make_shared<SharedBuffer>(encryptedS3Data);
            S3DataStore::WriteData(h_,
                                   path,
                                   buffer,
                                   encryptionSetting,
                                   completion);
        }

    } // namespace s3datastore
} // namespace hermit

