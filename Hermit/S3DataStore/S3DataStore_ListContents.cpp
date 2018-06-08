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

#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/AddTrailingSlash.h"
#include "S3PathToDataPath.h"
#include "S3DataPath.h"
#include "S3DataStore.h"

namespace hermit {
	namespace s3datastore {
		namespace S3DataStore_ListItems_Impl {
			
			//
			class ListObjectsCallback : public s3::ObjectKeyReceiver {
			public:
				//
				ListObjectsCallback(const datastore::ListDataStoreItemsItemCallbackPtr& itemCallback) :
				mItemCallback(itemCallback) {
				}
				
				//
				virtual bool OnOneKey(const HermitPtr& h_, const std::string& objectKey) override {
					datastore::DataPathPtr itemPath;
					if (!S3PathToDataPath(h_, objectKey, itemPath)) {
						NOTIFY_ERROR(h_, "S3PathToDataPath failed for objectKey:", objectKey);
						return true;
					}
					return mItemCallback->OnOneItem(h_, itemPath);
				}
				
				//
                datastore::ListDataStoreItemsItemCallbackPtr mItemCallback;
			};
			
            //
            class ListCompletion : public s3::S3CompletionBlock {
            public:
                //
                ListCompletion(const datastore::DataPathPtr& rootPath,
                               const datastore::ListDataStoreItemsCompletionPtr& completion) :
                mRootPath(rootPath),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, datastore::ListDataStoreItemsResult::kCanceled);
                        return;
                    }
                    if (result == s3::S3Result::kSuccess) {
                        mCompletion->Call(h_, datastore::ListDataStoreItemsResult::kSuccess);
                        return;
                    }
                    if (result == s3::S3Result::k403AccessDenied) {
                        NOTIFY_ERROR(h_, "k403AccessDenied for:", mRootPath);
                        mCompletion->Call(h_, datastore::ListDataStoreItemsResult::kPermissionDenied);
                        return;
                    }
                    if (result == s3::S3Result::k404NoSuchBucket) {
                        NOTIFY_ERROR(h_, "k404NoSuchBucket for:", mRootPath);
                        mCompletion->Call(h_, datastore::ListDataStoreItemsResult::kDataStoreMissing);
                        return;
                    }
                    NOTIFY_ERROR(h_, "mBucket->ListObjects failed for:", mRootPath, "result:", (int32_t)result);
                    mCompletion->Call(h_, datastore::ListDataStoreItemsResult::kError);
                }
                
                //
                datastore::DataPathPtr mRootPath;
                datastore::ListDataStoreItemsCompletionPtr mCompletion;
            };
            
		} // namespace S3DataStore_ListItems_Impl
        using namespace S3DataStore_ListItems_Impl;
		
		//
        void S3DataStore::ListItems(const HermitPtr& h_,
									const datastore::DataPathPtr& rootPath,
									const datastore::ListDataStoreItemsItemCallbackPtr& itemCallback,
									const datastore::ListDataStoreItemsCompletionPtr& completion) {
			S3DataPath& dataPath = static_cast<S3DataPath&>(*rootPath);			
            auto listCallback = std::make_shared<ListObjectsCallback>(itemCallback);
            auto listCompletion = std::make_shared<ListCompletion>(rootPath, completion);
			mBucket->ListObjects(h_, dataPath.mPath, listCallback, listCompletion);
		}
		
	} // namespace s3datastore
} // namespace hermit
