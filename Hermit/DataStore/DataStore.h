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

#ifndef DataStore_h
#define DataStore_h

#include <memory>
#include <string>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/SharedBuffer.h"
#include "DataPath.h"
#include "EncryptionSetting.h"

namespace hermit {
	namespace datastore {

		// parentPath, itemName
		DEFINE_CALLBACK_2A(ListDataStoreContentsItemCallback, DataPathPtr, std::string);
		
		//
		enum class ListDataStoreContentsResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kDataStoreMissing,
			kPermissionDenied,
			kError
		};
		
		//
		enum class ItemExistsInDataStoreStatus {
			kUnknown,
			kSuccess,
			kDataStoreMissing,
			kPermissionDenied,
			kCanceled,
			kError
		};
		
		//
		//
		DEFINE_CALLBACK_2A(ItemExistsInDataStoreCallback,
						   ItemExistsInDataStoreStatus,		// inStatus
						   bool);							// inExists
		
		//
		//
		class ItemExistsInDataStoreCallbackClass
		:
		public ItemExistsInDataStoreCallback {
		public:
			//
			ItemExistsInDataStoreCallbackClass()
			:
			mStatus(ItemExistsInDataStoreStatus::kUnknown),
			mExists(false) {
			}
			
			//
			bool Function(const ItemExistsInDataStoreStatus& inStatus, const bool& inExists) {
				mStatus = inStatus;
				mExists = inExists;
				return true;
			}
			
			//
			ItemExistsInDataStoreStatus mStatus;
			bool mExists;
		};
		
		
		//
		enum CreateDataStoreLocationIfNeededStatus {
			kCreateDataStoreLocationIfNeededStatus_Unknown,
			kCreateDataStoreLocationIfNeededStatus_Success,
			kCreateDataStoreLocationIfNeededStatus_ConflictAtPath,
			kCreateDataStoreLocationIfNeededStatus_StorageFull,
			kCreateDataStoreLocationIfNeededStatus_Error
		};
		
		
		//
		enum class LoadDataStoreDataStatus {
			kUnknown,
			kSuccess,
			kItemNotFound,
			kDownloadCapExceeded,
			kDataStoreMissing,
			kPermissionDenied,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(LoadDataStoreDataCompletionBlock, HermitPtr, LoadDataStoreDataStatus);

		//
		DEFINE_ASYNC_FUNCTION_1A(LoadDataStoreDataDataBlock, DataBuffer);
		
		//
		class LoadDataStoreDataData : public LoadDataStoreDataDataBlock {
		public:
			//
			LoadDataStoreDataData() {
			}
			
			//
			virtual void Call(const DataBuffer& inData) override {
				if ((inData.first != nullptr) && (inData.second > 0)) {
					mData.assign(inData.first, inData.second);
				}
			}
			
			//
			std::string mData;
		};
		typedef std::shared_ptr<LoadDataStoreDataData> LoadDataStoreDataDataPtr;
		
		//
		enum WriteDataStoreDataStatus {
			kWriteDataStoreDataStatus_Unknown,
			kWriteDataStoreDataStatus_Success,
			kWriteDataStoreDataStatus_AccessDenied,
			kWriteDataStoreDataStatus_TimedOut,
			kWriteDataStoreDataStatus_StorageFull,
			kWriteDataStoreDataStatus_Canceled,
			kWriteDataStoreDataStatus_Error
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(WriteDataStoreDataCompletionFunction, HermitPtr, WriteDataStoreDataStatus);
		
		//
		struct DataStore {
			//
			virtual ListDataStoreContentsResult ListContents(const HermitPtr& h_,
															 const DataPathPtr& inRootPath,
															 const ListDataStoreContentsItemCallbackRef& inItemCallback);
			
			//
			virtual void ItemExists(const HermitPtr& h_,
									const DataPathPtr& inItemPath,
									const ItemExistsInDataStoreCallbackRef& inCallback);
			
			//
			virtual CreateDataStoreLocationIfNeededStatus CreateLocationIfNeeded(const HermitPtr& h_, const DataPathPtr& inPath);

			//
			virtual void LoadData(const HermitPtr& h_,
								  const DataPathPtr& inPath,
								  const EncryptionSetting& inEncryptionSetting,
								  const LoadDataStoreDataDataBlockPtr& inDataBlock,
								  const LoadDataStoreDataCompletionBlockPtr& inCompletion);

			//
			virtual void WriteData(const HermitPtr& h_,
								   const DataPathPtr& inPath,
								   const SharedBufferPtr& inData,
								   const EncryptionSetting& inEncryptionSetting,
								   const WriteDataStoreDataCompletionFunctionPtr& inCompletionFunction);

			//
			virtual bool DeleteItem(const HermitPtr& h_, const DataPathPtr& inPath);

		protected:
			//
			virtual ~DataStore() = default;
		};
		typedef std::shared_ptr<DataStore> DataStorePtr;

	} // namespace datastore
} // namespace hermit

#endif
