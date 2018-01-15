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
#include "S3DataPath.h"
#include "S3DataStore.h"

namespace hermit {
	namespace s3datastore {
		
		namespace {
			
			//
			class ObjectCallback : public s3::ObjectKeyReceiver {
			public:
				//
				ObjectCallback() {
				}
				
				//
				virtual bool OnOneKey(const HermitPtr& h_, const std::string& inObjectKey) override {
					//	We only want the first match, since the target path may be a subset of additional
					//	items that will show up here.
					if (mObjectKey.empty()) {
						mObjectKey = inObjectKey;
					}
					// never need to be called more than once.
					return false;
				}
				
				//
				std::string mObjectKey;
			};
            typedef std::shared_ptr<ObjectCallback> ObjectCallbackPtr;
            
            //
            class ListCompletion : public s3::S3CompletionBlock {
            public:
                //
                ListCompletion(const std::string& dataPath,
                               const ObjectCallbackPtr& objectCallback,
                               const datastore::ItemExistsInDataStoreCompletionPtr& completion) :
                mDataPath(dataPath),
                mObjectCallback(objectCallback),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kCanceled, false);
                        return;
                    }
                    if (result == s3::S3Result::kSuccess) {
                        if (mObjectCallback->mObjectKey.empty()) {
                            mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kSuccess, false);
                        }
                        else if (mObjectCallback->mObjectKey == mDataPath) {
                            mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kSuccess, true);
                        }
                        else {
                            // A partial key match was found, but not an exact match.
                            // Check for the directory case.
                            std::string dataPathWithTrailingSlash;
                            string::AddTrailingSlash(mDataPath, dataPathWithTrailingSlash);
                            if (mObjectCallback->mObjectKey.find(dataPathWithTrailingSlash) == 0) {
                                mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kSuccess, true);
                            }
                            else {
                                mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kSuccess, false);
                            }
                        }
                        return;
                    }
                    if (result == s3::S3Result::k403AccessDenied) {
                        mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kPermissionDenied, false);
                        return;
                    }
                    if (result == s3::S3Result::k404NoSuchBucket) {
                        mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kDataStoreMissing, false);
                        return;
                    }
                    
                    NOTIFY_ERROR(h_,
                                 "ItemExistsInS3DataStore: mBucket->ListObjects failed, result:", (int32_t)result,
                                 "path:", mDataPath);
                    mCompletion->Call(h_, datastore::ItemExistsInDataStoreResult::kError, false);
                }
                
                //
                std::string mDataPath;
                ObjectCallbackPtr mObjectCallback;
                datastore::ItemExistsInDataStoreCompletionPtr mCompletion;
            };
            
		} // private namespace
		
		//
        void S3DataStore::ItemExists(const HermitPtr& h_,
									 const datastore::DataPathPtr& itemPath,
									 const datastore::ItemExistsInDataStoreCompletionPtr& completion) {			
			S3DataPath& dataPath = static_cast<S3DataPath&>(*itemPath);
            auto objectCallback = std::make_shared<ObjectCallback>();
            auto listCompletion = std::make_shared<ListCompletion>(dataPath.mPath, objectCallback, completion);
            mBucket->ListObjects(h_, dataPath.mPath, objectCallback, listCompletion);
		}
		
	} // namespace s3datastore
} // namespace hermit
