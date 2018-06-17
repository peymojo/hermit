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

#include <string>
#include "Hermit/File/CreateDirectoryParentChain.h"
#include "Hermit/File/GetFilePathParent.h"
#include "Hermit/File/WriteFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "FileDataStore.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		namespace FileDataStore_WriteData_Impl {
			
			//
			class Completion : public file::WriteFileDataCompletion {
			public:
				//
				Completion(const datastore::WriteDataStoreDataCompletionFunctionPtr& inCompletion) :
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::WriteFileDataResult& result) override {
					if (result == file::WriteFileDataResult::kCanceled) {
						mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
						return;
					}
					if (result == file::WriteFileDataResult::kDiskFull) {
                        mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kStorageFull);
						return;
					}
					if (result == file::WriteFileDataResult::kNoSuchFile) {
						mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kNoSuchFile);
						return;
					}
					if (result != file::WriteFileDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "WriteFileData failed.");
						mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kError);
						return;
					}
					mCompletion->Call(h_, datastore::WriteDataStoreDataResult::kSuccess);
				}
				
				//
				datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
			};
			
		} // namespace FileDataStore_WriteData_Impl
        using namespace FileDataStore_WriteData_Impl;
		
		//
        void FileDataStore::WriteData(const HermitPtr& h_,
                                      const datastore::DataPathPtr& path,
                                      const SharedBufferPtr& data,
                                      const datastore::EncryptionSetting& encryptionSetting,
                                      const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
			if (CHECK_FOR_ABORT(h_)) {
				completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
				return;
			}
			
			FilePathDataPath& fileDataPath = static_cast<FilePathDataPath&>(*path);
			
			file::FilePathPtr parentPath;
			file::GetFilePathParent(h_, fileDataPath.mFilePath, parentPath);
			if (parentPath == nullptr) {
				NOTIFY_ERROR(h_, "GetFilePathParent failed.");
				completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
				return;
			}
			auto createResult = file::CreateDirectoryParentChain(h_, parentPath);
			if (createResult != file::CreateDirectoryParentChainResult::kSuccess) {
				NOTIFY_ERROR(h_, "CreateDirectoryParentChain failed.");
				completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
				return;
			}
			
			auto writeCompletion = std::make_shared<Completion>(completion);
			file::WriteFileData(h_, fileDataPath.mFilePath, DataBuffer(data->Data(), data->Size()), writeCompletion);
		}
		
	} // namespace filedatastore
} // namespace hermit

