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
#include "Hermit/Encoding/CalculateDataCRC32.h"
#include "Hermit/File/StreamInFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "CalculateFileCRC32.h"

namespace hermit {
	namespace encoding_file {
		
		namespace {
			
			//
			class Reader : public DataProviderBlock {
			public:
				//
				Reader(file::FilePathPtr inFilePath) :
				mFilePath(inFilePath) {
				}
				
				//
				~Reader() {
				}
				
				//
				virtual void ProvideData(const HermitPtr& h_,
										 const DataHandlerBlockPtr& dataHandler,
										 const StreamResultBlockPtr& resultBlock) override {
					return file::StreamInFileData(h_, mFilePath, 1024 * 1024, dataHandler, resultBlock);
				}
				
				//
				file::FilePathPtr mFilePath;
			};
			
			//
			class Completion : public encoding::CalculateDataCRC32Completion {
			public:
				//
				Completion(const file::FilePathPtr& filePath, const CalculateFileCRC32CompletionPtr& completion) :
				mFilePath(filePath),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const encoding::CalculateDataCRC32Result& result,
								  const std::uint32_t& crc32) override {
					if (result == encoding::CalculateDataCRC32Result::kCanceled) {
						mCompletion->Call(h_, CalculateFileCRC32Result::kCanceled, 0);
						return;
					}
					if (result != encoding::CalculateDataCRC32Result::kSuccess) {
						NOTIFY_ERROR(h_, "CalculateFileCRC32: CalculateDataCRC32 failed, path:", mFilePath);
						mCompletion->Call(h_, CalculateFileCRC32Result::kError, 0);
						return;
					}
					mCompletion->Call(h_, CalculateFileCRC32Result::kSuccess, crc32);
				}

				//
				file::FilePathPtr mFilePath;
				CalculateFileCRC32CompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void CalculateFileCRC32(const HermitPtr& h_,
								const file::FilePathPtr& filePath,
								const CalculateFileCRC32CompletionPtr& completion) {
			auto reader = std::make_shared<Reader>(filePath);
			auto crcCompletion = std::make_shared<Completion>(filePath, completion);
			encoding::CalculateDataCRC32(h_, reader, crcCompletion);
		}
		
	} // namespace encoding_file
} // namespace hermit

