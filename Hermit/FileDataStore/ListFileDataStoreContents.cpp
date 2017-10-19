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

#include "Hermit/File/ListDirectoryContents.h"
#include "Hermit/Foundation/Notification.h"
#include "FilePathDataPath.h"
#include "ListFileDataStoreContents.h"

namespace hermit {
	namespace filedatastore {
		
		namespace {
			
			//
			class ListItemsCallback : public file::ListDirectoryContentsItemCallback {
			public:
				//
				ListItemsCallback(datastore::DataPathPtr inRootPath,
								  const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) :
				mRootPath(inRootPath),
				mItemCallback(inItemCallback) {
				}
				
				//
				virtual bool Function(const HermitPtr& h_,
									  const file::FilePathPtr& inParentFilePath,
									  const std::string& inItemName) override {
					return mItemCallback.Call(mRootPath, inItemName);
				}
				
				//
				datastore::DataPathPtr mRootPath;
				const datastore::ListDataStoreContentsItemCallbackRef& mItemCallback;
			};
			
		} // private namespace
		
		//
		datastore::ListDataStoreContentsResult
		ListFileDataStoreContents(const HermitPtr& h_,
								  const datastore::DataStorePtr& inDataStore,
								  const datastore::DataPathPtr& inRootPath,
								  const datastore::ListDataStoreContentsItemCallbackRef& inItemCallback) {
			
			FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*inRootPath);
			
			ListItemsCallback listItemsCallback(inRootPath, inItemCallback);
			auto result = file::ListDirectoryContents(h_, dataPath.mFilePath, false, listItemsCallback);
			if (result == file::ListDirectoryContentsResult::kCanceled) {
				return datastore::ListDataStoreContentsResult::kCanceled;
			}
			if (result == file::ListDirectoryContentsResult::kSuccess) {
				return datastore::ListDataStoreContentsResult::kSuccess;
			}
			if (result == file::ListDirectoryContentsResult::kDirectoryNotFound) {
				// we treat this as a success ... just no items there
				return datastore::ListDataStoreContentsResult::kSuccess;
			}
			if (result == file::ListDirectoryContentsResult::kPermissionDenied) {
				NOTIFY_ERROR(h_, "PermissionDenied for:", dataPath.mFilePath);
				return datastore::ListDataStoreContentsResult::kPermissionDenied;
			}
			NOTIFY_ERROR(h_, "ListDirectoryContents failed for:", dataPath.mFilePath);
			return datastore::ListDataStoreContentsResult::kError;
		}
		
	} // namespace filedatastore
} // namespace hermit
