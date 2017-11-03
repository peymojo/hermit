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

#include "Hermit/Encoding/AES256EncryptCBC.h"
#include "Hermit/Encoding/CreateInputVector.h"
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Notification.h"
#include "AES256EncryptedFileDataStore.h"
#include "WriteFileDataStoreData.h"
#include "WriteAES256EncryptedFileDataStoreData.h"

namespace hermit {
namespace filedatastore {

namespace {

	//
	void PerformWork(const HermitPtr& h_,
					 const datastore::DataStorePtr& inDataStore,
					 const datastore::DataPathPtr& inPath,
					 const SharedBufferPtr& inData,
					 const datastore::EncryptionSetting& inEncryptionSetting,
					 const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction) {
		if (CHECK_FOR_ABORT(h_)) {
			inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Canceled);
			return;
		}

		AES256EncryptedFileDataStore& dataStore = static_cast<AES256EncryptedFileDataStore&>(*inDataStore);

		std::string encryptedFileData;
		if (inEncryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
			encryptedFileData.assign(inData->Data(), inData->Size());
		}
		else {
			std::string inputVector;
			encoding::CreateInputVector(h_, 16, inputVector);

			encoding::AES256EncryptCBCCallbackClass dataCallback;
			encoding::AES256EncryptCBC(h_,
									   DataBuffer(inData->Data(), inData->Size()),
									   dataStore.mAESKey,
									   inputVector,
									   dataCallback);

			if (dataCallback.mStatus == encoding::kAES256EncryptCBC_Canceled) {
				inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Canceled);
				return;
			}
			if (dataCallback.mStatus != encoding::kAES256EncryptCBC_Success) {
				NOTIFY_ERROR(h_, "WriteAES256EncryptedFileDataStoreData: AES256EncryptCBC failed.");
				inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Error);
				return;
			}

			encryptedFileData = inputVector;
			encryptedFileData += dataCallback.mValue;
		}

		SharedBufferPtr buffer = std::make_shared<SharedBuffer>(encryptedFileData);
		WriteFileDataStoreData(h_,
							   inDataStore,
							   inPath,
							   buffer,
							   inEncryptionSetting,
							   inCompletionFunction);
	}

	//
	class Task : public AsyncTask {
	public:
		//
		Task(const HermitPtr& h_,
			 const datastore::DataStorePtr& inDataStore,
			 const datastore::DataPathPtr& inPath,
			 const SharedBufferPtr& inData,
			 const datastore::EncryptionSetting& inEncryptionSetting,
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
		datastore::EncryptionSetting mEncryptionSetting;
		datastore::WriteDataStoreDataCompletionFunctionPtr mCompletionFunction;
	};

} // private namespace

//
void WriteAES256EncryptedFileDataStoreData(const HermitPtr& h_,
										   const datastore::DataStorePtr& inDataStore,
										   const datastore::DataPathPtr& inPath,
										   const SharedBufferPtr& inData,
										   const datastore::EncryptionSetting& inEncryptionSetting,
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

