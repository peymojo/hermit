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
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/QueueAsyncTask.h"
#include "S3DataPath.h"
#include "S3DataStore.h"
#include "WriteS3DataStoreData.h"

namespace hermit {
namespace s3datastore {

namespace {

	//
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

		S3DataStore& dataStore = static_cast<S3DataStore&>(*inDataStore);
		S3DataPath& dataPath = static_cast<S3DataPath&>(*inPath);

		s3::PutS3ObjectCallbackClass callback;
		dataStore.mBucket->PutObject(h_,
									 dataPath.mPath,
									 DataBuffer(inData->Data(), inData->Size()),
									 dataStore.mUseReducedRedundancyStorage,
									 callback);

		datastore::WriteDataStoreDataStatus status = datastore::kWriteDataStoreDataStatus_Unknown;
		if (callback.mResult == s3::S3Result::kSuccess) {
			status = datastore::kWriteDataStoreDataStatus_Success;
		}
		else if (callback.mResult == s3::S3Result::kCanceled) {
			status = datastore::kWriteDataStoreDataStatus_Canceled;
		}
		else if (callback.mResult == s3::S3Result::k403AccessDenied) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::k403AccessDenied for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_AccessDenied;
		}
		else if ((callback.mResult == s3::S3Result::kS3InternalError) ||
				 (callback.mResult == s3::S3Result::k500InternalServerError) ||
				 (callback.mResult == s3::S3Result::k503ServiceUnavailable)) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: Server error for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_Error;
		}
		else if (callback.mResult == s3::S3Result::kHostNotFound) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::kHostNotFound for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_Error;
		}
		else if (callback.mResult == s3::S3Result::kNetworkConnectionLost) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::kNetworkConnectionLost for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_Error;
		}
		else if (callback.mResult == s3::S3Result::kNoNetworkConnection) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::kNoNetworkConnection for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_Error;
		}
		else if (callback.mResult == s3::S3Result::kTimedOut) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::kTimedOut for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_TimedOut;
		}
		else if (callback.mResult == s3::S3Result::kError) {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: S3Result::kError for:", dataPath.mPath);
			status = datastore::kWriteDataStoreDataStatus_Error;
		}
		else {
			NOTIFY_ERROR(h_, "WriteS3DataStoreData: Unexpected status for:", dataPath.mPath);
		}
		inCompletionFunction->Call(h_, status);
	}

	//
	//
	class Task
		:
		public AsyncTask
	{
	public:
		//
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
		//
		void PerformTask(
			const int32_t& inTaskID)
		{
			PerformWork(mH_,
						mDataStore,
						mPath,
						mData,
						mEncryptionSetting,
						mCompletionFunction);
		}
		
		//
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
//
void WriteS3DataStoreData(const HermitPtr& h_,
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
	if (!QueueAsyncTask(task, 10)) {
		NOTIFY_ERROR(h_, "QueueAsyncTask failed.");
		inCompletionFunction->Call(h_, datastore::kWriteDataStoreDataStatus_Error);
	}
}

} // namespace s3datastore
} // namespace hermit
