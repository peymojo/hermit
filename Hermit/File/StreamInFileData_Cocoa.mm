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
		namespace StreamInFileData_Impl {
			
			//
			class FileStreamer;
			typedef std::shared_ptr<FileStreamer> FileStreamerPtr;
			
			//
			class FileStreamer : public std::enable_shared_from_this<FileStreamer> {
			public:
				//
				FileStreamer(const FilePathPtr& filePath,
							 NSFileHandle* fileHandle,
							 uint64_t chunkSize,
							 const DataReceiverPtr& dataReceiver,
							 const DataCompletionPtr& completion) :
				mFilePath(filePath),
				mFileHandle(fileHandle),
				mChunkSize(chunkSize),
				mDataReceiver(dataReceiver),
				mCompletion(completion),
				mReadFinished(false) {
				}
				
				//
				class ReceiveCompletion : public DataCompletion {
				public:
					//
					ReceiveCompletion(const FileStreamerPtr& streamer) : mStreamer(streamer) {
					}
					
					//
					virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
						mStreamer->HandleReceiveResult(h_, result);
					}
					
					//
					FileStreamerPtr mStreamer;
				};
				
				//
				void StreamSomeData(const HermitPtr& h_) {
					@try {
						NSData* data = [mFileHandle readDataOfLength:(size_t)mChunkSize];
						uint64_t actualCount = [data length];
						const char* dataP = nullptr;
						if (actualCount > 0) {
							dataP = (const char*)[data bytes];
						}
						mReadFinished = (actualCount < mChunkSize);
						auto receiveCompletion = std::make_shared<ReceiveCompletion>(shared_from_this());
						mDataReceiver->Call(h_, DataBuffer(dataP, actualCount), mReadFinished, receiveCompletion);
					}
					@catch (NSException* ex) {
						NOTIFY_ERROR(h_,
									 "StreamInFileData: exception streaming:", mFilePath,
									 "name:", [[ex name] UTF8String],
									 "reason:", [[ex reason] UTF8String]);
						mCompletion->Call(h_, StreamDataResult::kError);
					}
				}
				
				//
				void HandleReceiveResult(const HermitPtr& h_, const StreamDataResult& result) {
					if (result == StreamDataResult::kCanceled) {
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "StreamInFileData: data receiver failed, path:", mFilePath);
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					if (mReadFinished) {
						[mFileHandle closeFile];
						mCompletion->Call(h_, result);
						return;
					}
					
					// Still more to read.
					StreamSomeData(h_);
				}
				
				//
				FilePathPtr mFilePath;
				NSFileHandle* mFileHandle;
				uint64_t mChunkSize;
				DataReceiverPtr mDataReceiver;
				DataCompletionPtr mCompletion;
				bool mReadFinished;
			};
			
		} // namespace StreamInFileData_Impl
		using namespace StreamInFileData_Impl;
		
		//
		void StreamInFileData(const HermitPtr& h_,
							  const FilePathPtr& filePath,
							  std::uint64_t chunkSize,
							  const DataReceiverPtr& dataReceiver,
							  const DataCompletionPtr& completion) {
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
					
					auto streamer = std::make_shared<FileStreamer>(filePath, fileHandle, chunkSize, dataReceiver, completion);
					streamer->StreamSomeData(h_);
				}
				@catch (NSException* ex) {
					NOTIFY_ERROR(h_,
								 "StreamInFileData: exception setting up:", filePath,
								 "name:", [[ex name] UTF8String],
								 "reason:", [[ex reason] UTF8String]);
					completion->Call(h_, StreamDataResult::kError);
				}
			}
		}
		
	} // namespace file
} // namespace hermit
