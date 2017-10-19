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
#include "FilePathDataPath.h"
#include "LoadFileDataStoreData.h"

namespace hermit {
	namespace filedatastore {
		
		namespace {
			
			//
			class DataBlock : public hermit::DataHandlerBlock {
			public:
				//
				virtual void HandleData(const hermit::HermitPtr& h_,
										const hermit::DataBuffer& data,
										bool isEndOfData,
										const hermit::StreamResultBlockPtr& resultBlock) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					resultBlock->Call(h_, hermit::StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<DataBlock> DataBlockPtr;

			//
			class LoadDataCompletion : public file::LoadFileDataCompletion {
			public:
				//
				LoadDataCompletion(const file::FilePathPtr& inFilePath,
								   const DataBlockPtr& data,
								   const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
								   const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) :
				mFilePath(inFilePath),
				mData(data),
				mDataBlock(inDataBlock),
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::LoadFileDataResult& result) override {
					if (result == file::LoadFileDataResult::kFileNotFound) {
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kItemNotFound);
						return;
					}
					if (result != file::LoadFileDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "LoadFileDataStoreData: LoadFileData failed for path:", mFilePath);
						mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kError);
						return;
					}
					mDataBlock->Call(DataBuffer(mData->mData.data(), mData->mData.size()));
					mCompletion->Call(h_, datastore::LoadDataStoreDataStatus::kSuccess);
					return;
				}
				
				//
				file::FilePathPtr mFilePath;
				DataBlockPtr mData;
				datastore::LoadDataStoreDataDataBlockPtr mDataBlock;
				datastore::LoadDataStoreDataCompletionBlockPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void LoadFileDataStoreData(const HermitPtr& h_,
								   const datastore::DataStorePtr& inDataStore,
								   const datastore::DataPathPtr& inPath,
								   const datastore::DataStoreEncryptionSetting& inEncryptionSetting,
								   const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
								   const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			FilePathDataPath& filePath = static_cast<FilePathDataPath&>(*inPath);
			auto dataBlock = std::make_shared<DataBlock>();
			auto loadCompletion = std::make_shared<LoadDataCompletion>(filePath.mFilePath,
																	   dataBlock,
																	   inDataBlock,
																	   inCompletion);
			file::LoadFileData(h_, filePath.mFilePath, dataBlock, loadCompletion);
		}
		
	} // namespace filedatastore
} // namespace hermit
