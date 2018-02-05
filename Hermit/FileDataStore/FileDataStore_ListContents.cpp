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
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		namespace FileDataStore_ListContents_Impl {
			
			//
			class ListItemsCallback : public file::ListDirectoryContentsItemCallback {
			public:
				//
				ListItemsCallback(datastore::DataPathPtr rootPath,
								  const datastore::ListDataStoreContentsItemCallbackPtr& itemCallback) :
				mRootPath(rootPath),
				mItemCallback(itemCallback) {
				}
				
				//
				virtual bool OnItem(const HermitPtr& h_,
									const file::ListDirectoryContentsResult& result,
									const file::FilePathPtr& parentFilePath,
									const std::string& itemName) override {
					return mItemCallback->OnOneItem(h_, mRootPath, itemName);
				}
				
				//
				datastore::DataPathPtr mRootPath;
                datastore::ListDataStoreContentsItemCallbackPtr mItemCallback;
			};
			
		} // namespace FileDataStore_ListContents_Impl
        using namespace FileDataStore_ListContents_Impl;
		
		//
        void FileDataStore::ListContents(const HermitPtr& h_,
                                         const datastore::DataPathPtr& rootPath,
                                         const datastore::ListDataStoreContentsItemCallbackPtr& itemCallback,
                                         const datastore::ListDataStoreContentsCompletionPtr& completion) {
			FilePathDataPath& dataPath = static_cast<FilePathDataPath&>(*rootPath);
			
			ListItemsCallback listItemsCallback(rootPath, itemCallback);
			auto result = file::ListDirectoryContents(h_, dataPath.mFilePath, false, listItemsCallback);
			if (result == file::ListDirectoryContentsResult::kCanceled) {
				completion->Call(h_, datastore::ListDataStoreContentsResult::kCanceled);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kSuccess) {
				completion->Call(h_, datastore::ListDataStoreContentsResult::kSuccess);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kDirectoryNotFound) {
				// we treat this as a success ... just no items there
				completion->Call(h_, datastore::ListDataStoreContentsResult::kSuccess);
                return;
			}
			if (result == file::ListDirectoryContentsResult::kPermissionDenied) {
				NOTIFY_ERROR(h_, "PermissionDenied for:", dataPath.mFilePath);
				completion->Call(h_, datastore::ListDataStoreContentsResult::kPermissionDenied);
                return;
			}
			NOTIFY_ERROR(h_, "ListDirectoryContents failed for:", dataPath.mFilePath);
			completion->Call(h_, datastore::ListDataStoreContentsResult::kError);
		}
		
	} // namespace filedatastore
} // namespace hermit
