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
#include "Hermit/Foundation/Notification.h"
#include "StreamOutFileData.h"
#include "WriteFileData.h"

namespace hermit {
	namespace file {
		namespace WriteFileData_Impl {

			//
			class StreamOutProvider : public DataProvider {
			public:
				//
				StreamOutProvider(const DataBuffer& inData) : mData(inData) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const DataReceiverPtr& dataReceiver,
								  const DataCompletionPtr& completion) override {
					dataReceiver->Call(h_, mData, true, completion);
				}
				
				//
				DataBuffer mData;
			};
			
			//
			class Completion : public DataCompletion {
			public:
				//
				Completion(const WriteFileDataCompletionPtr& completion) : mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
					if (result == StreamDataResult::kCanceled) {
						mCompletion->Call(h_, WriteFileDataResult::kCanceled);
						return;
					}
					if (result == StreamDataResult::kDiskFull) {
						mCompletion->Call(h_, WriteFileDataResult::kDiskFull);
						return;
					}
					if (result == StreamDataResult::kFileNotFound) {
						mCompletion->Call(h_, WriteFileDataResult::kNoSuchFile);
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "WriteFileData: StreamOutFileData failed.");
						mCompletion->Call(h_, WriteFileDataResult::kError);
						return;
					}
					mCompletion->Call(h_, WriteFileDataResult::kSuccess);
				}

				//
				WriteFileDataCompletionPtr mCompletion;
			};
			
		} // namespace WriteFileData_Impl
		using namespace WriteFileData_Impl;
		
		//
		void WriteFileData(const HermitPtr& h_,
						   const FilePathPtr& filePath,
						   const DataBuffer& data,
						   const WriteFileDataCompletionPtr& completion) {
			auto streamOutBlock = std::make_shared<StreamOutProvider>(data);
			auto streamCompletion = std::make_shared<Completion>(completion);
			StreamOutFileData(h_, filePath, streamOutBlock, streamCompletion);
		}
		
	} // namespace file
} // namespace hermit
