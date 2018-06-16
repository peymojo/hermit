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
#include "Hermit/Encoding/CalculateMD5FromStream.h"
#include "Hermit/File/GetFileDataSize.h"
#include "Hermit/File/FilePath.h"
#include "Hermit/File/StreamInFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "GetFileMD5WithProgress.h"

namespace hermit {
	namespace encoding_file {
		
		namespace {
			
			//
			class Progress {
			public:
				//
				Progress(uint64_t dataSize) : mDataSize(dataSize), mBytesProcessed(0), mLastPercentDone(0) {
				}
				
				//
				uint64_t mDataSize;
				uint64_t mBytesProcessed;
				int mLastPercentDone;
			};
			typedef std::shared_ptr<Progress> ProgressPtr;
						
			//
			class DataHandlerProxy : public DataHandlerBlock {
			public:
				//
				DataHandlerProxy(const DataHandlerBlockPtr& dataHandler, const ProgressPtr& progress) :
				mDataHandler(dataHandler),
				mProgress(progress) {
				}
				
				//
				virtual StreamDataResult HandleData(const HermitPtr& h_, const DataBuffer& data, bool isEndOfData) override {
					auto result = mDataHandler->HandleData(h_, data, isEndOfData);
					if (result == StreamDataResult::kCanceled) {
						return result;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "GetFileMD5WithProgress: mDataHandler returned an error.");
						return result;
					}
					
					mProgress->mBytesProcessed += data.second;
					double percent = mProgress->mBytesProcessed / (double)mProgress->mDataSize;
					int percentDone = (int)(percent * 100);
					if ((!isEndOfData) && (percentDone > 99)) {
						percentDone = 99;
					}
					if (percentDone > mProgress->mLastPercentDone) {
						if (CHECK_FOR_ABORT(h_)) {
							return StreamDataResult::kCanceled;
						}
						mProgress->mLastPercentDone = percentDone;
						
						// [???] not actually reporting the progress here any more, notification mechanism
						// should be used to do so
					}
					return StreamDataResult::kSuccess;
				}
				
				//
				ProgressPtr mProgress;
				DataHandlerBlockPtr mDataHandler;
			};
			
			//
			class DataStreamer : public DataProviderBlock {
			public:
				//
				DataStreamer(file::FilePathPtr inFilePath, uint64_t inDataSize) :
				mFilePath(inFilePath),
				mDataSize(inDataSize) {
				}
				
				//
				virtual void ProvideData(const HermitPtr& h_,
										 const DataHandlerBlockPtr& dataHandler,
										 const StreamResultBlockPtr& resultBlock) override {
					auto progress = std::make_shared<Progress>(mDataSize);
					auto dataHandlerProxy = std::make_shared<DataHandlerProxy>(dataHandler, progress);
					file::StreamInFileData(h_, mFilePath, 1024 * 1024, dataHandlerProxy, resultBlock);
				}
				
				//
				file::FilePathPtr mFilePath;
				uint64_t mDataSize;
			};
			
		} // private namespace
		
		//
		void GetFileMD5WithProgress(const HermitPtr& h_,
									const file::FilePathPtr& filePath,
									const encoding::CalculateMD5CompletionPtr& completion) {
			uint64_t dataSize = 0;
			if (!file::GetFileDataSize(h_, filePath, dataSize)) {
				NOTIFY_ERROR(h_, "GetFileMD5WithProgress: GetFileDataSize failed for:", filePath);
				completion->Call(h_, encoding::CalculateMD5Result::kError, "");
				return;
			}

			auto streamer = std::make_shared<DataStreamer>(filePath, dataSize);
			return encoding::CalculateMD5FromStream(h_, streamer, completion);
		}
		
	} // namespace encoding_file
} // namespace hermit
