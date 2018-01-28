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
#include "AES256EncryptedFileDataStore.h"

namespace hermit {
    namespace filedatastore {
        
        namespace {
            
            //
            class Task : public AsyncTask {
            public:
                //
                Task(const datastore::DataStorePtr& dataStore,
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
                    AES256EncryptedFileDataStore& dataStore = static_cast<AES256EncryptedFileDataStore&>(*mDataStore);
                    dataStore.DoWriteData(h_,
                                          mPath,
                                          mData,
                                          mEncryptionSetting,
                                          mCompletion);
                }
                
                //
                datastore::DataStorePtr mDataStore;
                datastore::DataPathPtr mPath;
                SharedBufferPtr mData;
                datastore::EncryptionSetting mEncryptionSetting;
                datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
            };
            
        } // private namespace
        
        //
        void AES256EncryptedFileDataStore::WriteData(const HermitPtr& h_,
                                                     const datastore::DataPathPtr& path,
                                                     const SharedBufferPtr& data,
                                                     const datastore::EncryptionSetting& encryptionSetting,
                                                     const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
            auto task = std::make_shared<Task>(shared_from_this(),
                                               path,
                                               data,
                                               encryptionSetting,
                                               completion);
            if (!QueueAsyncTask(h_, task, 15)) {
                NOTIFY_ERROR(h_, "QueueAsyncTask failed.");
                completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
            }
        }
        
        //
        void AES256EncryptedFileDataStore::DoWriteData(const HermitPtr& h_,
                                                       const datastore::DataPathPtr& path,
                                                       const SharedBufferPtr& data,
                                                       const datastore::EncryptionSetting& encryptionSetting,
                                                       const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
            if (CHECK_FOR_ABORT(h_)) {
                completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
                return;
            }
            
            std::string encryptedFileData;
            if (encryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
                encryptedFileData.assign(data->Data(), data->Size());
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
                    NOTIFY_ERROR(h_, "AES256EncryptCBC failed.");
                    completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
                    return;
                }
                
                encryptedFileData = inputVector;
                encryptedFileData += dataCallback.mValue;
            }
            
            SharedBufferPtr buffer = std::make_shared<SharedBuffer>(encryptedFileData);
            FileDataStore::WriteData(h_,
                                     path,
                                     buffer,
                                     encryptionSetting,
                                     completion);
        }

    } // namespace filedatastore
} // namespace hermit

