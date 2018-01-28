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

#include <stddef.h>
#include "Hermit/Foundation/Notification.h"
#include "CreateFilePathFromUTF8String.h"
#include "ReadFirstLineFromFile.h"
#include "ReadFirstLineFromUTF8FilePath.h"

namespace hermit {
	namespace file {
		namespace ReadFirstLineFromUTF8FilePath_Impl {

			//
			class ReadCompletion : public ReadFirstLineFromFileCompletion {
			public:
				//
				ReadCompletion(const ReadFirstLineFromUTF8FilePathCompletionPtr& completion) :
				mCompletion(completion) {
				}
				
				//
				void Call(const HermitPtr& h_,
						  const ReadFirstLineFromFileResult& result,
						  const std::string& line) {
					if (result == ReadFirstLineFromFileResult::kCanceled) {
						mCompletion->Call(h_, ReadFirstLineFromUTF8FilePathResult::kCanceled, "");
						return;
					}
					if (result == ReadFirstLineFromFileResult::kFileNotFound) {
						mCompletion->Call(h_, ReadFirstLineFromUTF8FilePathResult::kFileNotFound, "");
						return;
					}
					if (result != ReadFirstLineFromFileResult::kSuccess) {
						mCompletion->Call(h_, ReadFirstLineFromUTF8FilePathResult::kError, "");
						return;
					}
					mCompletion->Call(h_, ReadFirstLineFromUTF8FilePathResult::kSuccess, line);
				}
				
			private:
				//
				ReadFirstLineFromUTF8FilePathCompletionPtr mCompletion;
			};
			
		} // namespace ReadFirstLineFromUTF8FilePath_Impl
		using namespace ReadFirstLineFromUTF8FilePath_Impl;
		
		//
		void ReadFirstLineFromUTF8FilePath(const HermitPtr& h_,
										   const std::string& filePathUTF8,
										   const ReadFirstLineFromUTF8FilePathCompletionPtr& completion) {
			FilePathPtr filePath;
			CreateFilePathFromUTF8String(h_, filePathUTF8, filePath);
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "ReadFirstLineFromUTF8FilePath: CreateFilePathFromUTF8String failed for:", filePathUTF8);
				completion->Call(h_, ReadFirstLineFromUTF8FilePathResult::kError, "");
				return;
			}
			auto readCompletion = std::make_shared<ReadCompletion>(completion);
			ReadFirstLineFromFile(h_, filePath, readCompletion);
		}
		
	} // namespace file
} // namespace hermit
