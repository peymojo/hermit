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

#import <string>
#import <Foundation/Foundation.h>
#import <Foundation/FoundationErrors.h>
#import "Hermit/Foundation/Notification.h"
#import "DeleteFile.h"
#import "FileNotification.h"
#import "FilePathToCocoaPathString.h"
#import "StreamOutFileData.h"

namespace hermit {
	namespace file {
		namespace StreamOutFileData_Impl {
			
			//
			class DataWriter : public DataReceiver {
			public:
				//
				DataWriter(const FilePathPtr& filePath, NSFileHandle* fileHandle) :
				mFilePath(filePath),
				mFileHandle(fileHandle),
				mBytesWritten(0) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					if (data.second > 0) {
						const uint64_t kBlockSize = 1024 * 1024 * 64;
						
						uint64_t bytesWritten = 0;
						uint64_t bytesRemaining = data.second;
						while (bytesRemaining > 0) {
							uint64_t bytesToWrite = kBlockSize;
							if (bytesToWrite > bytesRemaining) {
								bytesToWrite = bytesRemaining;
							}
							
							const char* dataPtr = data.first + bytesWritten;
							NSData* data = [NSData dataWithBytesNoCopy:(void*)dataPtr length:(size_t)bytesToWrite freeWhenDone:NO];
							@try {
								[mFileHandle writeData:data];
							}
							@catch (NSException* exception) {
								//	Fugly... but does Cocoa provide a structured way of figuring out what the actual error is?
								if ([exception.reason containsString:@"No space left on device"]) {
									completion->Call(h_, StreamDataResult::kDiskFull);
									return;
								}

								NOTIFY_ERROR(h_,
											 "DataWriter: exception during writeData:", [exception.name UTF8String],
											 "reason:", [exception.reason UTF8String]);
								completion->Call(h_, StreamDataResult::kError);
								return;
							}
							
							mBytesWritten += bytesToWrite;
							BytesWrittenNotificationParam param(mFilePath, bytesToWrite);
							NOTIFY(h_, kBytesWrittenNotification, &param);
							
							bytesWritten += bytesToWrite;
							bytesRemaining -= bytesToWrite;
						}
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				FilePathPtr mFilePath;
				NSFileHandle* mFileHandle;
				uint64_t mBytesWritten;
			};
			typedef std::shared_ptr<DataWriter> DataWriterPtr;
			
			//
			class Completion : public DataCompletion {
			public:
				//
				Completion(const FilePathPtr& filePath,
						   NSFileHandle* fileHandle,
						   const DataWriterPtr& dataWriter,
						   const DataCompletionPtr& completion) :
				mFilePath(filePath),
				mFileHandle(fileHandle),
				mDataWriter(dataWriter),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
					bool deleteFile = false;
					if (result == StreamDataResult::kCanceled) {
						deleteFile = true;
					}
					else if (result == StreamDataResult::kDiskFull) {
						// no need to log this one, client can decide what to do.
						deleteFile = true;
					}
					else if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "StreamOutFileData: Error encountered while streaming data for:", mFilePath);
						deleteFile = true;
					}
					else {
						[mFileHandle truncateFileAtOffset:mDataWriter->mBytesWritten];
					}
					[mFileHandle closeFile];
					
					if (deleteFile) {
						DeleteFile(h_, mFilePath);
					}
					mCompletion->Call(h_, result);
				}
				
				//
				FilePathPtr mFilePath;
				NSFileHandle* mFileHandle;
				DataWriterPtr mDataWriter;
				DataCompletionPtr mCompletion;
			};
			
		} // namespace StreamOutFileData_Impl
		using namespace StreamOutFileData_Impl;
		
		//
		void StreamOutFileData(const HermitPtr& h_,
							   const FilePathPtr& filePath,
							   const DataProviderPtr& dataProvider,
							   const DataCompletionPtr& completion) {
			@autoreleasepool {
				try {
					std::string pathUTF8;
					FilePathToCocoaPathString(h_, filePath, pathUTF8);
					NSString* pathString = [NSString stringWithUTF8String:pathUTF8.c_str()];
					NSURL* pathURL = [NSURL fileURLWithPath:pathString];
					NSError* error = nil;
					NSFileHandle* fileHandle = [NSFileHandle fileHandleForWritingToURL:pathURL error:&error];
					if (fileHandle == nullptr) {
						error = nil;
						[[NSData data] writeToFile:pathString options:0 error:&error];
						if (error == nil) {
							fileHandle = [NSFileHandle fileHandleForWritingToURL:pathURL error:&error];
						}
					}
					if (fileHandle == nullptr) {
						if ((error != nil) &&
							[error.domain isEqualToString:NSCocoaErrorDomain] &&
							(error.code == NSFileWriteOutOfSpaceError)) {
							completion->Call(h_, StreamDataResult::kDiskFull);
							return;
						}
						if ((error != nil) &&
							[error.domain isEqualToString:NSCocoaErrorDomain] &&
							(error.code == NSFileNoSuchFileError)) {
							completion->Call(h_, StreamDataResult::kFileNotFound);
							return;
						}
						
						if (error != nil) {
							NSInteger errorCode = [error code];
							NOTIFY_ERROR(h_,
										 "StreamOutFileData: NSFileHandle fileHandleForWritingToURL failed for:", filePath,
										 "error:", [[error localizedDescription] UTF8String],
										 "domain:", [[error domain] UTF8String],
										 "error code:", (int32_t)errorCode);
						}
						else {
							NOTIFY_ERROR(h_, "StreamOutFileData: NSFileHandle fileHandleForWritingToURL failed for:", filePath);
						}
						
						completion->Call(h_, StreamDataResult::kError);
						return;
					}
					
					auto writer = std::make_shared<DataWriter>(filePath, fileHandle);
					auto providerCompletion = std::make_shared<Completion>(filePath, fileHandle, writer, completion);
					dataProvider->Call(h_, writer, providerCompletion);
				}
				catch (NSException* ex) {
					NOTIFY_ERROR(h_,
								 "StreamOutFileData: Outer Exception processing file:", filePath,
								 "name:", [[ex name] UTF8String],
								 "reason:", [[ex reason] UTF8String]);
					completion->Call(h_, StreamDataResult::kError);
				}
			}
		}
		
	} // namespace file
} // namespace hermit

