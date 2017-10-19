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

#include <map>
#include <stddef.h>
#include <string>
#include <vector>
#include "Hermit/Encoding/KeyPhraseToKeyWithValues.h"
#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/File/FilePath.h"
#include "Hermit/File/WriteFileData.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/JSON/DataValueToJSON.h"
#include "Hermit/Value/Value.h"
#include "KeyPhraseToKeyWithJSONFile.h"

namespace hermit {
	namespace encoding_file {
		
		namespace {

			//
			class Completion : public file::WriteFileDataCompletion {
			public:
				//
				Completion(const std::string& aesKey,
						   const file::FilePathPtr& jsonFilePath,
						   const KeyPhraseToKeyWithJSONFileCompletionPtr& inCompletion) :
				mAESKey(aesKey),
				mJSONFilePath(jsonFilePath),
				mCompletion(inCompletion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::WriteFileDataResult& result) override {
					if (result == file::WriteFileDataResult::kCanceled) {
						mCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kCanceled, "", nullptr);
						return;
					}
					if (result != file::WriteFileDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "KeyPhraseToKeyWithJSONFile: WriteFileData failed.");
						mCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kError, "", nullptr);
						return;
					}
					mCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kSuccess, mAESKey, mJSONFilePath);
				}
				
				//
				std::string mAESKey;
				file::FilePathPtr mJSONFilePath;
				KeyPhraseToKeyWithJSONFileCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void KeyPhraseToKeyWithJSONFile(const HermitPtr& h_,
										const std::string& inKeyPhrase,
										const file::FilePathPtr& inParentDirectory,
										const std::string& inKeyJSONFileName,
										const KeyPhraseToKeyWithJSONFileCompletionPtr& inCompletion) {
			file::FilePathPtr keyJSONFilePath;
			file::AppendToFilePath(h_, inParentDirectory, inKeyJSONFileName, keyJSONFilePath);
			if (keyJSONFilePath == nullptr) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithJSONFile: AppendToFilePath failed, inParentDirectory:", inParentDirectory);
				NOTIFY_ERROR(h_, "inKeyJSONFileName:", inKeyJSONFileName);
				inCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kError, "", nullptr);
				return;
			}
			
			std::string aesKey;
			value::ObjectValuePtr keyValues;
			auto status = encoding::KeyPhraseToKeyWithValues(h_, inKeyPhrase, aesKey, keyValues);
			if (status == encoding::KeyPhraseToKeyStatus::kCanceled) {
				inCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kCanceled, "", nullptr);
				return;
			}
			if (status != encoding::KeyPhraseToKeyStatus::kSuccess) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithJSONFile: KeyPhraseToKeyWithValues failed.");
				inCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kError, "", nullptr);
				return;
			}
			
			std::string jsonString;
			json::DataValueToJSON(h_, keyValues, jsonString);
			if (jsonString.empty()) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithJSONFile: DataValueToJSON failed.");
				inCompletion->Call(h_, KeyPhraseToKeyWithJSONFileResult::kError, "", nullptr);
				return;
			}
			
			auto completion = std::make_shared<Completion>(aesKey, keyJSONFilePath, inCompletion);
			file::WriteFileData(h_, keyJSONFilePath, DataBuffer(jsonString.data(), jsonString.size()), completion);
		}
		
	} // namespace encoding_file
} // namespace hermit
