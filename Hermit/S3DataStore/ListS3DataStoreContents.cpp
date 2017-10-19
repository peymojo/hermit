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
#include "ListS3DataStoreContents.h"

namespace hermit {
	namespace s3datastore {
		
		namespace {
			
			//
			class ListObjectsCallback : public s3::ObjectKeyReceiver {
			public:
				//
				ListObjectsCallback(const datastore::DataPathPtr& inRootPath,
									const std::string& inBaseS3Prefix,
									const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) :
				mRootPath(inRootPath),
				mBaseS3Prefix(inBaseS3Prefix),
				mCallback(inItemCallback) {
				}
				
				//
				virtual bool operator()(const std::string& inObjectKey) override {
					std::string partialObjectKey(inObjectKey);
					if (partialObjectKey.find(mBaseS3Prefix) == 0) {
						partialObjectKey = partialObjectKey.substr(mBaseS3Prefix.size());
					}
					mCallback.Call(mRootPath, partialObjectKey);
					return true;
				}
				
				//
				datastore::DataPathPtr mRootPath;
				std::string mBaseS3Prefix;
				const datastore::ListDataStoreContentsItemCallbackRef& mCallback;
			};
			
		} // private namespace
		
		//
		datastore::ListDataStoreContentsResult
		ListS3DataStoreContents(const HermitPtr& h_,
								const datastore::DataStorePtr& inDataStore,
								const datastore::DataPathPtr& inRootPath,
								const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) {
			
			S3DataStore& dataStore = static_cast<S3DataStore&>(*inDataStore);
			S3DataPath& dataPath = static_cast<S3DataPath&>(*inRootPath);
			
			std::string pathWithTrailingSlash;
			string::AddTrailingSlash(dataPath.mPath, pathWithTrailingSlash);
			
			ListObjectsCallback listCallback(inRootPath, pathWithTrailingSlash, inItemCallback);
			auto result = dataStore.mBucket->ListObjects(h_, dataPath.mPath, listCallback);
			
			if (result == s3::S3Result::kCanceled) {
				return datastore::ListDataStoreContentsResult::kCanceled;
			}
			if (result == s3::S3Result::kSuccess) {
				return datastore::ListDataStoreContentsResult::kSuccess;
			}
			if (result == s3::S3Result::k403AccessDenied) {
				NOTIFY_ERROR(h_, "k403AccessDenied for:", dataPath.mPath);
				return datastore::ListDataStoreContentsResult::kPermissionDenied;
			}
			if (result == s3::S3Result::k404NoSuchBucket) {
				NOTIFY_ERROR(h_, "k404NoSuchBucket for:", dataPath.mPath);
				return datastore::ListDataStoreContentsResult::kDataStoreMissing;
			}
			NOTIFY_ERROR(h_, "mBucket->ListObjects failed for:", dataPath.mPath, "result:", (int32_t)result);
			return datastore::ListDataStoreContentsResult::kError;
		}
		
	} // namespace s3datastore
} // namespace hermit
