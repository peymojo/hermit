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
		namespace S3DataStore_ListContents_Impl {
			
			//
			class ListObjectsCallback : public s3::ObjectKeyReceiver {
			public:
				//
				ListObjectsCallback(const datastore::DataPathPtr& rootPath,
									const std::string& baseS3Prefix,
									const datastore::ListDataStoreContentsItemCallbackPtr& itemCallback) :
				mRootPath(rootPath),
				mBaseS3Prefix(baseS3Prefix),
				mItemCallback(itemCallback) {
				}
				
				//
				virtual bool OnOneKey(const HermitPtr& h_, const std::string& objectKey) override {
					std::string partialObjectKey(objectKey);
					if (partialObjectKey.find(mBaseS3Prefix) == 0) {
						partialObjectKey = partialObjectKey.substr(mBaseS3Prefix.size());
					}
					return mItemCallback->OnOneItem(h_, mRootPath, partialObjectKey);
				}
				
				//
				datastore::DataPathPtr mRootPath;
				std::string mBaseS3Prefix;
                datastore::ListDataStoreContentsItemCallbackPtr mItemCallback;
			};
			
            //
            class ListCompletion : public s3::S3CompletionBlock {
            public:
                //
                ListCompletion(const datastore::DataPathPtr& rootPath,
                               const datastore::ListDataStoreContentsCompletionPtr& completion) :
                mRootPath(rootPath),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, datastore::ListDataStoreContentsResult::kCanceled);
                        return;
                    }
                    if (result == s3::S3Result::kSuccess) {
                        mCompletion->Call(h_, datastore::ListDataStoreContentsResult::kSuccess);
                        return;
                    }
                    if (result == s3::S3Result::k403AccessDenied) {
                        NOTIFY_ERROR(h_, "k403AccessDenied for:", mRootPath);
                        mCompletion->Call(h_, datastore::ListDataStoreContentsResult::kPermissionDenied);
                        return;
                    }
                    if (result == s3::S3Result::k404NoSuchBucket) {
                        NOTIFY_ERROR(h_, "k404NoSuchBucket for:", mRootPath);
                        mCompletion->Call(h_, datastore::ListDataStoreContentsResult::kDataStoreMissing);
                        return;
                    }
                    NOTIFY_ERROR(h_, "mBucket->ListObjects failed for:", mRootPath, "result:", (int32_t)result);
                    mCompletion->Call(h_, datastore::ListDataStoreContentsResult::kError);
                }
                
                //
                datastore::DataPathPtr mRootPath;
                datastore::ListDataStoreContentsCompletionPtr mCompletion;
            };
            
		} // namespace S3DataStore_ListContents_Impl
        using namespace S3DataStore_ListContents_Impl;
		
		//
        void S3DataStore::ListContents(const HermitPtr& h_,
                                       const datastore::DataPathPtr& rootPath,
                                       const datastore::ListDataStoreContentsItemCallbackPtr& itemCallback,
                                       const datastore::ListDataStoreContentsCompletionPtr& completion) {
			S3DataPath& dataPath = static_cast<S3DataPath&>(*rootPath);
			
			std::string pathWithTrailingSlash;
			string::AddTrailingSlash(dataPath.mPath, pathWithTrailingSlash);
            auto listCallback = std::make_shared<ListObjectsCallback>(rootPath, pathWithTrailingSlash, itemCallback);
            auto listCompletion = std::make_shared<ListCompletion>(rootPath, completion);
			mBucket->ListObjects(h_, dataPath.mPath, listCallback, listCompletion);
		}
		
	} // namespace s3datastore
} // namespace hermit
