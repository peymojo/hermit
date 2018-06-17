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
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/SharedBuffer.h"
#include "DataPath.h"
#include "EncryptionSetting.h"

namespace hermit {
	namespace datastore {

        //
        class ListDataStoreItemsItemCallback {
        protected:
            //
            ~ListDataStoreItemsItemCallback() = default;
            
        public:
            //
            virtual bool OnOneItem(const HermitPtr& h_, const DataPathPtr& itemPath) = 0;
        };
        typedef std::shared_ptr<ListDataStoreItemsItemCallback> ListDataStoreItemsItemCallbackPtr;

		//
		enum class ListDataStoreItemsResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kDataStoreMissing,
			kPermissionDenied,
			kError
		};
		
        //
        DEFINE_ASYNC_FUNCTION_2A(ListDataStoreItemsCompletion, HermitPtr, ListDataStoreItemsResult);
        
        //
		enum class ItemExistsInDataStoreResult {
			kUnknown,
			kSuccess,
			kDataStoreMissing,
			kPermissionDenied,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(ItemExistsInDataStoreCompletion,
                                 HermitPtr,
                                 ItemExistsInDataStoreResult,		// inResult
                                 bool);						    	// inExists
		
        //
        DEFINE_ASYNC_FUNCTION_2A(LoadDataStoreDataDataBlock,
                                 HermitPtr,
                                 DataBuffer);
        
        //
        class LoadDataStoreDataData : public LoadDataStoreDataDataBlock {
        public:
            //
            LoadDataStoreDataData() {
            }
            
            //
            virtual void Call(const HermitPtr& h_, const DataBuffer& inData) override {
                if ((inData.first != nullptr) && (inData.second > 0)) {
                    mData.assign(inData.first, inData.second);
                }
            }
            
            //
            std::string mData;
        };
        typedef std::shared_ptr<LoadDataStoreDataData> LoadDataStoreDataDataPtr;

        //
		enum class LoadDataStoreDataResult {
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
		DEFINE_ASYNC_FUNCTION_2A(LoadDataStoreDataCompletionBlock,
                                 HermitPtr,
                                 LoadDataStoreDataResult);
		
		//
		enum class WriteDataStoreDataResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kAccessDenied,
			kTimedOut,
			kStorageFull,
			kNoSuchFile,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(WriteDataStoreDataCompletionFunction,
                                 HermitPtr,
                                 WriteDataStoreDataResult);
		
        //
        enum class DeleteDataStoreItemResult {
            kUnknown,
            kSuccess,
            kCanceled,
            kError
        };
        
        //
        DEFINE_ASYNC_FUNCTION_2A(DeleteDataStoreItemCompletion,
                                 HermitPtr,
                                 DeleteDataStoreItemResult);
        
		//
		struct DataStore {
			//
			virtual void ListItems(const HermitPtr& h_,
								   const DataPathPtr& rootPath,
								   const ListDataStoreItemsItemCallbackPtr& itemCallback,
								   const ListDataStoreItemsCompletionPtr& completion);
			
			//
			virtual void ItemExists(const HermitPtr& h_,
									const DataPathPtr& itemPath,
									const ItemExistsInDataStoreCompletionPtr& completion);
			
			//
			virtual void LoadData(const HermitPtr& h_,
								  const DataPathPtr& path,
								  const EncryptionSetting& encryptionSetting,
								  const LoadDataStoreDataDataBlockPtr& dataBlock,
								  const LoadDataStoreDataCompletionBlockPtr& completion);

			//
			virtual void WriteData(const HermitPtr& h_,
								   const DataPathPtr& path,
								   const SharedBufferPtr& data,
								   const EncryptionSetting& encryptionSetting,
								   const WriteDataStoreDataCompletionFunctionPtr& completion);

			//
			virtual void DeleteItem(const HermitPtr& h_,
                                    const DataPathPtr& inPath,
                                    const DeleteDataStoreItemCompletionPtr& completion);

		protected:
			//
			virtual ~DataStore() = default;
		};
		typedef std::shared_ptr<DataStore> DataStorePtr;

	} // namespace datastore
} // namespace hermit

#endif
