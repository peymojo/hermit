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
		namespace ReadUTF8File_Impl {
			
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
			class LoadCompletion : public LoadFileDataCompletion {
			public:
				//
				LoadCompletion(const ReceiverPtr& receiver,
							   const ReadUTF8FileLineBlockPtr& lineBlock,
							   const ReadUTF8FileCompletionPtr& completion) :
				mReceiver(receiver),
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
					
					std::string utf8FileData(mReceiver->mData);
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
				ReceiverPtr mReceiver;
				ReadUTF8FileLineBlockPtr mLineBlock;
				ReadUTF8FileCompletionPtr mCompletion;
			};
			
		} // namespace ReadUTF8File_Impl
		using namespace ReadUTF8File_Impl;

		//
		void ReadUTF8File(const HermitPtr& h_,
						  const FilePathPtr& filePath,
						  const ReadUTF8FileLineBlockPtr& lineBlock,
						  const ReadUTF8FileCompletionPtr& completion) {
			auto receiver = std::make_shared<Receiver>();
			auto loadCompletion = std::make_shared<LoadCompletion>(receiver, lineBlock, completion);
			LoadFileData(h_, filePath, receiver, loadCompletion);
		}
		
	} // namespace file
} // namespace hermit
