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
#include "FileDataStore.h"
#include "FilePathToDataPath.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		namespace FileDataStore_ListContents_Impl {
			
			//
			class ListItemsCallback : public file::ListDirectoryContentsItemCallback {
			public:
				//
				ListItemsCallback(const datastore::ListDataStoreItemsItemCallbackPtr& itemCallback) :
				mItemCallback(itemCallback) {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const file::ListDirectoryContentsResult& result,
									const file::FilePathPtr& parentFilePath,
									const std::string& itemName) override {
					datastore::DataPathPtr parentPath;
					if (!FilePathToDataPath(h_, parentFilePath, parentPath)) {
						NOTIFY_ERROR(h_, "FilePathToDataPath failed for:", parentFilePath);
						return true;
					}
					datastore::DataPathPtr itemPath;
					if (!parentPath->AppendPathComponent(h_, itemName, itemPath)) {
						NOTIFY_ERROR(h_, "parentPath->AppendPathComponent failed for:", itemName);
						return true;
					}
					
					return mItemCallback->OnOneItem(h_, itemPath);
				}
				
				//
                datastore::ListDataStoreItemsItemCallbackPtr mItemCallback;
			};
			
		} // namespace FileDataStore_ListContents_Impl
        using namespace FileDataStore_ListContents_Impl;
		
		//
        void FileDataStore::ListItems(const HermitPtr& h_,
									  const datastore::DataPathPtr& rootPath,
									  const datastore::ListDataStoreItemsItemCallbackPtr& itemCallback,
									  const datastore::ListDataStoreItemsCompletionPtr& completion) {
			FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*rootPath);
			
			ListItemsCallback listItemsCallback(itemCallback);
			auto result = file::ListDirectoryContents(h_, dataPath.mFilePath, true, listItemsCallback);
			if (result == file::ListDirectoryContentsResult::kCanceled) {
				completion->Call(h_, datastore::ListDataStoreItemsResult::kCanceled);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kSuccess) {
				completion->Call(h_, datastore::ListDataStoreItemsResult::kSuccess);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kDirectoryNotFound) {
				// we treat this as a success ... just no items there
				completion->Call(h_, datastore::ListDataStoreItemsResult::kSuccess);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kPermissionDenied) {
				NOTIFY_ERROR(h_, "PermissionDenied for:", dataPath.mFilePath);
				completion->Call(h_, datastore::ListDataStoreItemsResult::kPermissionDenied);
                return;
			}
			NOTIFY_ERROR(h_, "ListDirectoryContents failed for:", dataPath.mFilePath);
			completion->Call(h_, datastore::ListDataStoreItemsResult::kError);
		}
		
	} // namespace filedatastore
} // namespace hermit
