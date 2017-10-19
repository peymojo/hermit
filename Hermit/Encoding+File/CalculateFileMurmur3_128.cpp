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

#include "Hermit/Encoding/CalculateMurmur3_128.h"
#include "Hermit/File/StreamInFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "CalculateFileMurmur3_128.h"

namespace hermit {
	namespace encoding_file {
		
		namespace {

			//
			class Streamer : public DataProviderBlock {
			public:
				//
				Streamer(const file::FilePathPtr& inFilePath) : mFilePath(inFilePath) {
				}
				
				//
				virtual void ProvideData(const HermitPtr& h_,
										 const DataHandlerBlockPtr& dataHandler,
										 const StreamResultBlockPtr& resultBlock) override {
					file::StreamInFileData(h_, mFilePath, 1024 * 1024, dataHandler, resultBlock);
				}
				
				//
				file::FilePathPtr mFilePath;
			};
			
		} // private namespace
		
		//
		void CalculateFileMurmur3_128(const HermitPtr& h_,
									  const file::FilePathPtr& filePath,
									  const encoding::CalculateMurmurCompletionPtr& completion) {
			auto streamer = std::make_shared<Streamer>(filePath);
			encoding::CalculateMurmur3_128(h_, streamer, completion);
		}
		
	} // namespace encoding_file
} // namespace hermit
