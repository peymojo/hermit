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
#include "Hermit/File/WriteFileData.h"
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Notification.h"
#include "FileDataStore.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		namespace FileDataStore_WriteData_Impl {
			
			//
			class Completion : public file::WriteFileDataCompletion {
			public:
				//
				Completion(const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletion) :
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::WriteFileDataResult& result) override {
					if (result == file::WriteFileDataResult::kDiskFull) {
                        mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kStorageFull);
						return;
					}
					if (result != file::WriteFileDataResult::kSuccess) {
						mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kError);
						return;
					}
					mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kSuccess);
				}
				
				//
				datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
			};
			
			//
			void PerformWork(const HermitPtr& h_,
							 const datastore::DataStorePtr& inDataStore,
							 const datastore::DataPathPtr& inPath,
							 const SharedBufferPtr& inData,
							 const datastore::EncryptionSetting& inEncryptionSetting,
							 const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
				if (CHECK_FOR_ABORT(h_)) {
					inCompletionFunction->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
					return;
				}
				
				FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*inPath);
				auto completion = std::make_shared<Completion>(inCompletionFunction);
				file::WriteFileData(h_, dataPath.mFilePath, DataBuffer(inData->Data(), inData->Size()), completion);
			}
			
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
					PerformWork(h_,
								mDataStore,
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
			
		} // namespace FileDataStore_WriteData_Impl
        using namespace FileDataStore_WriteData_Impl;
		
		//
        void FileDataStore::WriteData(const HermitPtr& h_,
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
		
	} // namespace filedatastore
} // namespace hermit

