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
#include <stdlib.h>
#include <string>
#include <vector>
#include "Hermit/Encoding/ValidateKeyPhraseWithValues.h"
#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/File/FilePath.h"
#include "Hermit/File/LoadFileData.h"
#include "Hermit/JSON/EnumerateRootJSONValuesCallback.h"
#include "Hermit/JSON/JSONToDataValue.h"
#include "Hermit/Value/Value.h"
#include "ValidateKeyPhraseWithJSONFile.h"

namespace hermit {
	namespace encoding_file {
		
		namespace {
			
			//
			typedef std::map<std::string, value::ValuePtr> ValueMap;
			typedef std::vector<value::ValuePtr> ValueVector;
			
			//
			class DataBlock : public hermit::DataHandlerBlock {
			public:
				//
				virtual void HandleData(const hermit::HermitPtr& h_,
										const hermit::DataBuffer& data,
										bool isEndOfData,
										const hermit::StreamResultBlockPtr& resultBlock) override {
					if (data.second > 0) {
						mData.append(data.first, data.second);
					}
					resultBlock->Call(h_, hermit::StreamDataResult::kSuccess);
				}
				
				//
				std::string mData;
			};
			typedef std::shared_ptr<DataBlock> DataBlockPtr;

			//
			class LoadCompletion : public file::LoadFileDataCompletion {
			public:
				//
				LoadCompletion(const DataBlockPtr& data,
							   const std::string& keyPhrase,
							   const file::FilePathPtr& keyJSONFilePath,
							   const ValidateKeyPhraseWithJSONFileCompletionPtr& completion) :
				mData(data),
				mKeyPhrase(keyPhrase),
				mKeyJSONFilePath(keyJSONFilePath),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const file::LoadFileDataResult& result) override {
					if (result == file::LoadFileDataResult::kFileNotFound) {
						mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kFileNotFound, "");
						return;
					}
					if (result != file::LoadFileDataResult::kSuccess) {
						NOTIFY_ERROR(h_,
									 "ValidateKeyPhraseWithJSONFile: LoadFileData failed, keyJSONFilePath:",
									 mKeyJSONFilePath);
						mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kError, "");
						return;
					}
					
					value::ValuePtr value;
					uint64_t bytesConsumed = 0;
					if (!json::JSONToDataValue(h_, mData->mData.data(), mData->mData.size(), value, bytesConsumed)) {
						NOTIFY_ERROR(h_, "ValidateKeyPhraseWithJSONFile: JSONToDataValue failed, keyJSONFilePath:", mKeyJSONFilePath);
						mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kError, "");
						return;
					}
					if (value->GetDataType() != value::DataType::kObject) {
						NOTIFY_ERROR(h_, "ValidateKeyPhraseWithJSONFile: json data type not kObject, path:", mKeyJSONFilePath);
						mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kError, "");
						return;
					}
					auto values = std::static_pointer_cast<value::ObjectValue>(value);
					std::string aesKey;
					if (!encoding::ValidateKeyPhraseWithValues(h_, mKeyPhrase, values, aesKey)) {
						mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kKeyPhraseIncorrect, "");
						return;
					}
					mCompletion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kSuccess, aesKey);
				}
				
				//
				DataBlockPtr mData;
				std::string mKeyPhrase;
				file::FilePathPtr mKeyJSONFilePath;
				ValidateKeyPhraseWithJSONFileCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void ValidateKeyPhraseWithJSONFile(const HermitPtr& h_,
										   const std::string& keyPhrase,
										   const file::FilePathPtr& parentDirectory,
										   const std::string& keyJSONFileName,
										   const ValidateKeyPhraseWithJSONFileCompletionPtr& completion) {
			file::FilePathPtr keyJSONFilePath;
			file::AppendToFilePath(h_, parentDirectory, keyJSONFileName, keyJSONFilePath);
			if (keyJSONFilePath == nullptr) {
				NOTIFY_ERROR(h_,
							 "ValidateKeyPhraseWithJSONFile: AppendToFilePath failed, parentDirectory:",
							 parentDirectory,
							 "keyJSONFileName:",
							 keyJSONFileName);
				completion->Call(h_, ValidateKeyPhraseWithJSONFileResult::kError, "");
				return;
			}
			
			auto dataBlock = std::make_shared<DataBlock>();
			auto loadCompletion = std::make_shared<LoadCompletion>(dataBlock,
																   keyPhrase,
																   keyJSONFilePath,
																   completion);
			file::LoadFileData(h_, keyJSONFilePath, dataBlock, loadCompletion);
		}
		
	} // namespace encoding_file
} // namespace hermit
