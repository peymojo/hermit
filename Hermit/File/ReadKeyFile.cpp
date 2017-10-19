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
#include "ReadFirstLineFromFile.h"
#include "ReadKeyFile.h"

namespace hermit {
	namespace file {
		
		namespace {

			//
			class FirstLineCompletion : public ReadFirstLineFromFileCompletion {
			public:
				//
				FirstLineCompletion(const ReadKeyFileCompletionPtr& completion) :
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const ReadFirstLineFromFileResult& result,
								  const std::string& line) {
					if (result == ReadFirstLineFromFileResult::kCanceled) {
						mCompletion->Call(h_, ReadKeyFileResult::kCanceled, "");
						return;
					}
					if (result == ReadFirstLineFromFileResult::kFileNotFound) {
						mCompletion->Call(h_, ReadKeyFileResult::kFileNotFound, "");
						return;
					}
					if (result != ReadFirstLineFromFileResult::kSuccess) {
						mCompletion->Call(h_, ReadKeyFileResult::kError, "");
						return;
					}
					mCompletion->Call(h_, ReadKeyFileResult::kSuccess, line);
				}
								  
				//
				ReadKeyFileCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void ReadKeyFile(const HermitPtr& h_,
						 const FilePathPtr& filePath,
						 const ReadKeyFileCompletionPtr& completion) {
			auto lineCompletion = std::make_shared<FirstLineCompletion>(completion);
			ReadFirstLineFromFile(h_, filePath, lineCompletion);
		}
		
	} // namespace file
} // namespace hermit
