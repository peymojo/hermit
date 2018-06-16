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
#include "LoadFileData.h"
#include "ReadUTF8File.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			class DataBlock : public hermit::DataHandlerBlock {
			public:
				//
				virtual StreamDataResult HandleData(const hermit::HermitPtr& h_,
													const hermit::DataBuffer& data,
													bool isEndOfData) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					return hermit::StreamDataResult::kSuccess;
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<DataBlock> DataBlockPtr;

			//
			class LoadCompletion : public LoadFileDataCompletion {
			public:
				//
				LoadCompletion(const DataBlockPtr& data,
							   const ReadUTF8FileLineBlockPtr& lineBlock,
							   const ReadUTF8FileCompletionPtr& completion) :
				mData(data),
				mLineBlock(lineBlock),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const LoadFileDataResult& result) override {
					if (result == LoadFileDataResult::kFileNotFound) {
						mCompletion->Call(h_, ReadUTF8FileResult::kFileNotFound);
						return;
					}
					if (result != LoadFileDataResult::kSuccess) {
						mCompletion->Call(h_, ReadUTF8FileResult::kError);
						return;
					}
					
					std::string utf8FileData(mData->mData);
					std::string::size_type begin = 0;
					while (true) {
						std::string::size_type end = utf8FileData.find("\n", begin);
						if (end == std::string::npos) {
							if (utf8FileData.size() > begin) {
								if (!mLineBlock->WithLine(h_, utf8FileData.substr(begin))) {
									break;
								}
							}
							break;
						}
						if (!mLineBlock->WithLine(h_, utf8FileData.substr(begin, end - begin))) {
							break;
						}
						begin = end + 1;
					}
					mCompletion->Call(h_, ReadUTF8FileResult::kSuccess);
				}
				
				//
				DataBlockPtr mData;
				ReadUTF8FileLineBlockPtr mLineBlock;
				ReadUTF8FileCompletionPtr mCompletion;
			};
			
		} // private namespace

		//
		void ReadUTF8File(const HermitPtr& h_,
						  const FilePathPtr& filePath,
						  const ReadUTF8FileLineBlockPtr& lineBlock,
						  const ReadUTF8FileCompletionPtr& completion) {
			auto dataBlock = std::make_shared<DataBlock>();
			auto loadCompletion = std::make_shared<LoadCompletion>(dataBlock, lineBlock, completion);
			LoadFileData(h_, filePath, dataBlock, loadCompletion);
		}
		
	} // namespace file
} // namespace hermit
