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
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/QueueAsyncTask.h"
#include "FilePathDataPath.h"
#include "WriteFileDataStoreData.h"

namespace hermit {
	namespace filedatastore {
		
		namespace {
			
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
						mCompletion->Call(h_, datastore::kWriteDataStoreDataStatus_StorageFull);
						return;
					}
					if (result != file::WriteFileDataResult::kSuccess) {
						mCompletion->Call(h_, datastore::kWriteDataStoreDataStatus_Error);
						return;
					}
					mCompletion->Call(h_, datastore::kWriteDataStoreDataStatus_Success);
				}
				
				//
				datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
			};
			
			//
			void PerformWork(const HermitPtr& h_,
							 const datastore::DataStorePtr& inDataStore,
							 const datastore::DataPathPtr& inPath,
							 const SharedBufferPtr& inData,
							 const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
							 const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
				if (CHECK_FOR_ABORT(h_)) {
					inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Canceled);
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
				Task(const HermitPtr& h_,
					 const datastore::DataStorePtr& inDataStore,
					 const datastore::DataPathPtr& inPath,
					 const SharedBufferPtr& inData,
					 const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
					 const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mDataStore(inDataStore),
				mPath(inPath),
				mData(inData),
				mEncryptionSetting(inEncryptionSetting),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				void PerformTask(const int32_t& inTaskID) {
					PerformWork(mH_,
								mDataStore,
								mPath,
								mData,
								mEncryptionSetting,
								mCompletionFunction);
				}
				
				//
				HermitPtr mH_;
				datastore::DataStorePtr mDataStore;
				datastore::DataPathPtr mPath;
				SharedBufferPtr mData;
				datastore::DataStoreEncryptionSetting mEncryptionSetting;
				datastore::WriteDataStoreDataCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		void WriteFileDataStoreData(const HermitPtr& h_,
									const datastore::DataStorePtr& inDataStore,
									const datastore::DataPathPtr& inPath,
									const SharedBufferPtr& inData,
									const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
									const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
			auto task = std::make_shared<Task>(h_,
											   inDataStore,
											   inPath,
											   inData,
											   inEncryptionSetting,
											   inCompletionFunction);
			if (!QueueAsyncTask(task, 15)) {
				NOTIFY_ERROR(h_, "QueueAsyncTask failed.");
				inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Error);
			}
		}
		
	} // namespace filedatastore
} // namespace hermit
