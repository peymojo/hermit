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

#import <Foundation/Foundation.h>
#import <Foundation/FoundationErrors.h>
#import <string>
#import <sys/errno.h>
#import "Hermit/Foundation/AsyncTaskQueue.h"
#import "Hermit/Foundation/Notification.h"
#import "FilePathToCocoaPathString.h"
#import "StreamInFileData.h"

namespace hermit {
	namespace file {
		
		namespace {
			
			//
			class FileStreamer : public std::enable_shared_from_this<FileStreamer> {
				//
				typedef std::shared_ptr<FileStreamer> FileStreamerPtr;
				
				//
				class Completion : public StreamResultBlock {
				public:
					//
					Completion(const FileStreamerPtr& fileStreamer) : mFileStreamer(fileStreamer) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, StreamDataResult result) override {
						mFileStreamer->ChunkComplete(h_, result);
					}
					
					//
					FileStreamerPtr mFileStreamer;
				};

			public:
				//
				FileStreamer(const FilePathPtr& filePath,
							 NSFileHandle* fileHandle,
							 std::uint64_t chunkSize,
							 const DataHandlerBlockPtr& dataHandler,
							 const StreamResultBlockPtr& completion) :
				mFilePath(filePath),
				mFileHandle(fileHandle),
				mChunkSize(chunkSize),
				mIsLastChunk(false),
				mDataHandler(dataHandler),
				mCompletion(completion) {
				}
				
				//
				class StreamTask : public AsyncTask {
				public:
					//
					StreamTask(const HermitPtr& h_, const FileStreamerPtr& streamer) : mH_(h_), mStreamer(streamer) {
					}
					
					//
					virtual void PerformTask(const int32_t& taskId) override {
						mStreamer->PerformStreamTask(mH_);
					}
					
					//
					HermitPtr mH_;
					FileStreamerPtr mStreamer;
				};
				
				//
				void StreamNextChunk(const HermitPtr& h_) {
					auto task = std::make_shared<StreamTask>(h_, shared_from_this());
					if (!QueueAsyncTask(task, 1)) {
						NOTIFY_ERROR(h_, "StreamInFileData: QueueAsyncTask failed.");
						mCompletion->Call(h_, StreamDataResult::kError);
						return;
					}
				}
				//
				void PerformStreamTask(const HermitPtr& h_) {
					@autoreleasepool {
						@try {
							NSData* data = [mFileHandle readDataOfLength:(size_t)mChunkSize];
							uint64_t actualCount = [data length];
							const char* dataP = nullptr;
							if (actualCount > 0) {
								dataP = (const char*)[data bytes];
							}
							
							auto completion = std::make_shared<Completion>(shared_from_this());
							mIsLastChunk = (actualCount < mChunkSize);
							mDataHandler->HandleData(h_, DataBuffer(dataP, actualCount), mIsLastChunk, completion);
						}
						@catch (NSException* ex) {
							NOTIFY_ERROR(h_,
										 "StreamInFileData: inner exception:", mFilePath,
										 "name:", [[ex name] UTF8String],
										 "reason:", [[ex reason] UTF8String]);
							mCompletion->Call(h_, StreamDataResult::kError);
						}
					}
				}
				
				//
				void ChunkComplete(const HermitPtr& h_, StreamDataResult result) {
					if (result == StreamDataResult::kCanceled) {
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "StreamInFileData: inDataHandler failed, path:", mFilePath);
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					if (mIsLastChunk) {
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					StreamNextChunk(h_);
				}

				//
				FilePathPtr mFilePath;
				NSFileHandle* mFileHandle;
				std::uint64_t mChunkSize;
				bool mIsLastChunk;
				DataHandlerBlockPtr mDataHandler;
				StreamResultBlockPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void StreamInFileData(const HermitPtr& h_,
							  const FilePathPtr& filePath,
							  std::uint64_t chunkSize,
							  const DataHandlerBlockPtr& dataHandler,
							  const StreamResultBlockPtr& completion) {
			@autoreleasepool {
				@try {
					std::string pathUTF8;
					FilePathToCocoaPathString(h_, filePath, pathUTF8);
					NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
					NSURL* pathURL = [NSURL fileURLWithPath:pathString];
					NSError* error = nil;
					NSFileHandle* fileHandle = [NSFileHandle fileHandleForReadingFromURL:pathURL error:&error];
					if (fileHandle == nil) {
						NOTIFY_ERROR(h_, "StreamInFileData: fileHandleForReadingFromURL failed, path:", pathUTF8);
						if (error != nil) {
							NSInteger errorCode = [error code];
							NOTIFY_ERROR(h_, "-- error code:", (int32_t)errorCode);
							NOTIFY_ERROR(h_, "-- error:", [[error localizedDescription] UTF8String]);
						}
						completion->Call(h_, StreamDataResult::kError);
						return;
					}
					if (chunkSize == 0) {
						chunkSize = 1024 * 1024;
					}
					auto streamer = std::make_shared<FileStreamer>(filePath, fileHandle, chunkSize, dataHandler, completion);
					streamer->StreamNextChunk(h_);
				}
				@catch (NSException* ex) {
					NOTIFY_ERROR(h_, "StreamInFileData: outer exception:", filePath);
					NOTIFY_ERROR(h_, "-- name:", [[ex name] UTF8String]);
					NOTIFY_ERROR(h_, "-- reason:", [[ex reason] UTF8String]);
					completion->Call(h_, StreamDataResult::kError);
				}
			}
		}
		
	} // namespace file
} // namespace hermit
