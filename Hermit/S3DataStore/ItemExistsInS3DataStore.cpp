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
#include "ItemExistsInS3DataStore.h"

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
				virtual bool operator()(const std::string& inObjectKey) override {
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
			
		} // private namespace
		
		//
		//
		void ItemExistsInS3DataStore(const HermitPtr& h_,
									 const datastore::DataStorePtr& inDataStore,
									 const datastore::DataPathPtr& inItemPath,
									 const datastore::ItemExistsInDataStoreCallbackRef& inCallback) {
			
			S3DataStore& dataStore = static_cast<S3DataStore&>(*inDataStore);
			S3DataPath& dataPath = static_cast<S3DataPath&>(*inItemPath);
			
			ObjectCallback objectCallback;
			auto result = dataStore.mBucket->ListObjects(h_, dataPath.mPath, objectCallback);
			if (result == s3::S3Result::kCanceled) {
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kCanceled, false);
				return;
			}
			if (result == s3::S3Result::kSuccess) {
				if (objectCallback.mObjectKey.empty()) {
					inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, false);
				}
				else if (objectCallback.mObjectKey == dataPath.mPath) {
					inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, true);
				}
				else {
					//	A partial key match was found, but not an exact match.
					//	Check for the directory case.
					std::string dataPathWithTrailingSlash;
					string::AddTrailingSlash(dataPath.mPath, dataPathWithTrailingSlash);
					if (objectCallback.mObjectKey.find(dataPathWithTrailingSlash) == 0) {
						inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, true);
					}
					else {
						inCallback.Call(datastore::ItemExistsInDataStoreStatus::kSuccess, false);
					}
				}
				return;
			}
			if (result == s3::S3Result::k403AccessDenied) {
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kPermissionDenied, false);
				return;
			}
			if (result == s3::S3Result::k404NoSuchBucket) {
				inCallback.Call(datastore::ItemExistsInDataStoreStatus::kDataStoreMissing, false);
				return;
			}
			
			NOTIFY_ERROR(h_,
						 "ItemExistsInS3DataStore: mBucket->ListObjects failed, result:", (int32_t)result,
						 "path:", dataPath.mPath);
			inCallback.Call(datastore::ItemExistsInDataStoreStatus::kError, false);
		}
		
	} // namespace s3datastore
} // namespace hermit
