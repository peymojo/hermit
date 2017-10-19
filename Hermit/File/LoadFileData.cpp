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
#include <errno.h>
#include <stdlib.h>
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "GetFilePathUTF8String.h"
#include "LoadFileData.h"

namespace hermit {
	namespace file {
		
		//
		class DataLoader : public std::enable_shared_from_this<DataLoader> {
			//
			typedef std::shared_ptr<DataLoader> DataLoaderPtr;

		public:
			//
			DataLoader(const std::string& filePathUTF8,
					   const DataHandlerBlockPtr& dataHandler,
					   const LoadFileDataCompletionPtr& completion) :
			mFilePathUTF8(filePathUTF8),
			mDataHandler(dataHandler),
			mCompletion(completion),
			mFileHandle(nullptr),
			mBuf(nullptr),
			mBufSize(0),
			mDone(false) {
			}
			
			//
			~DataLoader() {
				if (mBuf != nullptr) {
					::free(mBuf);
				}
				if (mFileHandle != nullptr) {
					::fclose(mFileHandle);
				}
			}
			
			//
			class ResultBlock : public StreamResultBlock {
			public:
				//
				ResultBlock(const DataLoaderPtr& dataLoader) : mDataLoader(dataLoader) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, StreamDataResult result) override {
					mDataLoader->DataHandled(h_, result);
				}
				
				//
				DataLoaderPtr mDataLoader;
			};
			
			//
			void LoadNextChunk(const HermitPtr& h_) {
				if (mFileHandle == nullptr) {
					mFileHandle = ::fopen(mFilePathUTF8.c_str(), "rb");
					if (mFileHandle == nullptr) {
						int err = errno;
						if (err == ENOENT) {
							mCompletion->Call(h_, LoadFileDataResult::kFileNotFound);
							return;
						}
						NOTIFY_ERROR(h_, "LoadFileData: fopen failed, err:", err);
						mCompletion->Call(h_, LoadFileDataResult::kError);
						return;
					}
					
					const int kBufferSize = 16 * 1024 * 1024;
					mBuf = (char*)malloc(kBufferSize);
					if (mBuf == nullptr) {
						NOTIFY_ERROR(h_, "LoadFileData: malloc failed.");
						mCompletion->Call(h_, LoadFileDataResult::kError);
						return;
					}
					mBufSize = kBufferSize;
				}
				
				size_t bytesRead = ::fread(mBuf, 1, mBufSize, mFileHandle);
				int err = ::ferror(mFileHandle);
				if (err != 0) {
					NOTIFY_ERROR(h_, "LoadFileData: ferror:", err);
					mCompletion->Call(h_, LoadFileDataResult::kError);
					return;
				}
				auto resultBlock = std::make_shared<ResultBlock>(shared_from_this());
				mDataHandler->HandleData(h_, DataBuffer(mBuf, bytesRead), false, resultBlock);
			}
			
			//
			void DataHandled(const HermitPtr& h_, StreamDataResult result) {
				if (result == StreamDataResult::kCanceled) {
					mCompletion->Call(h_, LoadFileDataResult::kCanceled);
					return;
				}
				if (result != StreamDataResult::kSuccess) {
					NOTIFY_ERROR(h_, "LoadFileData: result != StreamDataResult::kSuccess");
					mCompletion->Call(h_, LoadFileDataResult::kError);
					return;
				}
				if (mDone) {
					mCompletion->Call(h_, LoadFileDataResult::kSuccess);
					return;
				}
				if (::feof(mFileHandle)) {
					auto resultBlock = std::make_shared<ResultBlock>(shared_from_this());
					mDone = true;
					mDataHandler->HandleData(h_, DataBuffer("", 0), true, resultBlock);
					return;
				}
				LoadNextChunk(h_);
			}
			
			//
			std::string mFilePathUTF8;
			DataHandlerBlockPtr mDataHandler;
			LoadFileDataCompletionPtr mCompletion;
			FILE* mFileHandle;
			char* mBuf;
			size_t mBufSize;
			bool mDone;
		};
		
		//
		void LoadFileData(const HermitPtr& h_,
						  const FilePathPtr& filePath,
						  const DataHandlerBlockPtr& dataHandler,
						  const LoadFileDataCompletionPtr& completion) {
			std::string pathUTF8;
			GetFilePathUTF8String(h_, filePath, pathUTF8);
			auto dataLoader = std::make_shared<DataLoader>(pathUTF8, dataHandler, completion);
			dataLoader->LoadNextChunk(h_);
		}
		
	} // namespace file
} // namespace hermit
