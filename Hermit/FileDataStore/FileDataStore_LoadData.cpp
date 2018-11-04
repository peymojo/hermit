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

#include "Hermit/File/LoadFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "FileDataStore.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		namespace FileDataStore_LoadData_Impl {
			
			//
			class Receiver : public DataReceiver {
			public:
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<Receiver> ReceiverPtr;

			//
			class LoadDataCompletion : public file::LoadFileDataCompletion {
			public:
				//
				LoadDataCompletion(const file::FilePathPtr& filePath,
								   const ReceiverPtr& dataReceiver,
								   const datastore::LoadDataStoreDataDataBlockPtr& dataBlock,
								   const datastore::LoadDataStoreDataCompletionBlockPtr& completion) :
				mFilePath(filePath),
				mDataReceiver(dataReceiver),
				mDataBlock(dataBlock),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::LoadFileDataResult& result) override {
					if (result == file::LoadFileDataResult::kFileNotFound) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kItemNotFound);
						return;
					}
					if (result != file::LoadFileDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "LoadFileDataStoreData: LoadFileData failed for path:", mFilePath);
						mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
						return;
					}
					mDataBlock->Call(h_, DataBuffer(mDataReceiver->mData.data(), mDataReceiver->mData.size()));
					mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kSuccess);
					return;
				}
				
				//
				file::FilePathPtr mFilePath;
				ReceiverPtr mDataReceiver;
				datastore::LoadDataStoreDataDataBlockPtr mDataBlock;
				datastore::LoadDataStoreDataCompletionBlockPtr mCompletion;
			};
			
		} // namespace FileDataStore_LoadData_Impl
        using namespace FileDataStore_LoadData_Impl;
		
		//
        void FileDataStore::LoadData(const HermitPtr& h_,
                                     const datastore::DataPathPtr& path,
                                     const datastore::EncryptionSetting& encryptionSetting,
                                     const datastore::LoadDataStoreDataDataBlockPtr& dataBlock,
                                     const datastore::LoadDataStoreDataCompletionBlockPtr& completion) {
			FilePathDataPath& filePath = static_cast<FilePathDataPath&>(*path);
			auto fileReceiver = std::make_shared<Receiver>();
			auto fileCompletion = std::make_shared<LoadDataCompletion>(filePath.mFilePath,
																	   fileReceiver,
																	   dataBlock,
																	   completion);
			file::LoadFileData(h_, filePath.mFilePath, fileReceiver, fileCompletion);
		}
		
	} // namespace filedatastore
} // namespace hermit
