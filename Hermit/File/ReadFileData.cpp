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
#include "FilePathToCocoaPathString.h"
#include "ReadFileData.h"

namespace hermit {
	namespace file {
		
		//
		class DataLoader : public std::enable_shared_from_this<DataLoader> {
			//
			typedef std::shared_ptr<DataLoader> DataLoaderPtr;

		public:
			//
			DataLoader(const std::string& filePathUTF8,
					   const DataReceiverPtr& dataReceiver,
					   const DataCompletionPtr& completion) :
			mFilePathUTF8(filePathUTF8),
			mDataReceiver(dataReceiver),
			mCompletion(completion),
			mFileHandle(nullptr),
			mBuf(nullptr),
			mBufSize(0),
			mStreamResult(StreamDataResult::kSuccess),
			mDone(false) {
				pthread_mutex_init(&mMutex, nullptr);
				pthread_cond_init(&mCondition, nullptr);
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
			void ReadData(const HermitPtr& h_) {
				mFileHandle = ::fopen(mFilePathUTF8.c_str(), "rb");
				if (mFileHandle == nullptr) {
					int err = errno;
					if (err == ENOENT) {
						mCompletion->Call(h_, StreamDataResult::kFileNotFound);
						return;
					}
					NOTIFY_ERROR(h_, "ReadFileData: fopen failed, err:", err);
					mCompletion->Call(h_, StreamDataResult::kError);
					return;
				}
					
				const int kBufferSize = 16 * 1024 * 1024;
				mBuf = (char*)malloc(kBufferSize);
				if (mBuf == nullptr) {
					NOTIFY_ERROR(h_, "ReadFileData: malloc failed.");
					mCompletion->Call(h_, StreamDataResult::kError);
					return;
				}
				mBufSize = kBufferSize;
				
				while (true) {
					pthread_mutex_lock(&mMutex);
					while (mStreamResult == StreamDataResult::kUnknown) {
						pthread_cond_wait(&mCondition, &mMutex);
					}
					pthread_mutex_unlock(&mMutex);

					if (mStreamResult == StreamDataResult::kCanceled) {
						break;
					}
					if (mStreamResult != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "ReadFileData: result != StreamDataResult::kSuccess");
						break;
					}
					if (mDone) {
						break;
					}
	
					size_t bytesRead = 0;
					if (::feof(mFileHandle)) {
						mDone = true;
					}
					else {
						bytesRead = ::fread(mBuf, 1, mBufSize, mFileHandle);
						int err = ::ferror(mFileHandle);
						if (err != 0) {
							NOTIFY_ERROR(h_, "ReadFileData: ferror:", err);
							mStreamResult = StreamDataResult::kError;
							break;
						}
					}
					mStreamResult = StreamDataResult::kUnknown;
					auto receiveCompletion = std::make_shared<ReceiveCompletion>(shared_from_this());
					mDataReceiver->Call(h_, DataBuffer(mBuf, bytesRead), mDone, receiveCompletion);
				}
				
				mCompletion->Call(h_, mStreamResult);
			}
			
			//
			class ReceiveCompletion : public DataCompletion {
			public:
				//
				ReceiveCompletion(const DataLoaderPtr& loader) : mLoader(loader) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
					mLoader->HandleReceiveResult(h_, result);
				}
				
				//
				DataLoaderPtr mLoader;
			};
			
			//
			void HandleReceiveResult(const HermitPtr& h_, const StreamDataResult& result) {
				pthread_mutex_lock(&mMutex);
				mStreamResult = result;
				pthread_cond_signal(&mCondition);
				pthread_mutex_unlock(&mMutex);
			}

			//
			std::string mFilePathUTF8;
			DataReceiverPtr mDataReceiver;
			DataCompletionPtr mCompletion;
			FILE* mFileHandle;
			char* mBuf;
			size_t mBufSize;
			StreamDataResult mStreamResult;
			bool mDone;
			pthread_mutex_t mMutex;
			pthread_cond_t mCondition;
		};
		
		//
		void ReadFileData(const HermitPtr& h_,
						  const FilePathPtr& filePath,
						  const DataReceiverPtr& dataReceiver,
						  const DataCompletionPtr& completion) {
			std::string pathUTF8;
			FilePathToCocoaPathString(h_, filePath, pathUTF8);
			auto dataLoader = std::make_shared<DataLoader>(pathUTF8, dataReceiver, completion);
			dataLoader->ReadData(h_);
		}
		
	} // namespace file
} // namespace hermit
