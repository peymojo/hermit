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
#include "ReadUTF8File.h"
#include "ReadFirstLineFromFile.h"

namespace hermit {
	namespace file {
		
		namespace {

			//
			class LineBlock : public ReadUTF8FileLineBlock {
			public:
				//
				virtual bool WithLine(const HermitPtr& h_, const std::string& line) {
					mLine = line;

					//	We only want the first line.
					return false;
				}

				//
				std::string mLine;
			};
			typedef std::shared_ptr<LineBlock> LineBlockPtr;
			
			//
			class ReadFileCompletion : public ReadUTF8FileCompletion {
			public:
				//
				ReadFileCompletion(const LineBlockPtr& lineBlock,
								   const ReadFirstLineFromFileCompletionPtr& completion) :
				mLineBlock(lineBlock),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const ReadUTF8FileResult& result) override {
					if (result == ReadUTF8FileResult::kCanceled) {
						mCompletion->Call(h_, ReadFirstLineFromFileResult::kCanceled, "");
						return;
					}
					if (result == ReadUTF8FileResult::kFileNotFound) {
						mCompletion->Call(h_, ReadFirstLineFromFileResult::kFileNotFound, "");
						return;
					}
					if (result != ReadUTF8FileResult::kSuccess) {
						mCompletion->Call(h_, ReadFirstLineFromFileResult::kError, "");
						return;
					}
					mCompletion->Call(h_, ReadFirstLineFromFileResult::kSuccess, mLineBlock->mLine);
				}
				
				//
				LineBlockPtr mLineBlock;
				ReadFirstLineFromFileCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void ReadFirstLineFromFile(const HermitPtr& h_,
								   const FilePathPtr& filePath,
								   const ReadFirstLineFromFileCompletionPtr& completion) {
			auto lineBlock = std::make_shared<LineBlock>();
			auto readCompletion = std::make_shared<ReadFileCompletion>(lineBlock, completion);
			ReadUTF8File(h_, filePath, lineBlock, readCompletion);
		}
		
	} // namespace file
} // namespace hermit
