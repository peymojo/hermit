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
#include "Hermit/Encoding/ValidateKeyPhraseWithValues.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/JSON/JSONToDataValue.h"
#include "Hermit/String/AddTrailingSlash.h"
#include "Hermit/Value/Value.h"
#include "ValidateKeyPhraseWithS3JSON.h"

namespace hermit {
	namespace encoding_s3bucket {
		
		//
		class CompletionBlock : public s3::S3CompletionBlock {
		public:
			//
			CompletionBlock(const std::string& inKeyPhrase,
							const std::string& inKeystorePath,
							const s3::GetS3ObjectResponsePtr& inResponse,
							const ValidateKeyPhraseWithS3JSONCompletionPtr& inCompletion) :
			mKeyPhrase(inKeyPhrase),
			mKeystorePath(inKeystorePath),
			mResponse(inResponse),
			mCompletion(inCompletion) {
			}
			
			//
			virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
				if (result == s3::S3Result::kCanceled) {
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kCanceled, "");
					return;
				}
				if (result == s3::S3Result::k404EntityNotFound) {
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kObjectNotFound, "");
					return;
				}
				if (result != s3::S3Result::kSuccess) {
					NOTIFY_ERROR(h_, "ValidateKeyPhraseWithS3JSON: GetObjectFromS3Bucket failed, keyJSON path:", mKeystorePath);
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kError, "");
					return;
				}
				
				value::ValuePtr values;
				uint64_t bytesConsumed = 0;
				if (!json::JSONToDataValue(h_, mResponse->mData.data(), mResponse->mData.size(), values, bytesConsumed)) {
					NOTIFY_ERROR(h_, "ValidateKeyPhraseWithS3JSON: JSONToDataValue failed, keyJSON path:", mKeystorePath);
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kError, "");
					return;
				}
				if (values->GetDataType() != value::DataType::kObject) {
					NOTIFY_ERROR(h_, "ValidateKeyPhraseWithS3JSON: json data type not kObject, path:", mKeystorePath);
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kError, "");
					return;
				}
				auto objectValues = std::static_pointer_cast<value::ObjectValue>(values);
				std::string aesKey;
				if (!encoding::ValidateKeyPhraseWithValues(h_, mKeyPhrase, objectValues, aesKey)) {
					mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kKeyPhraseIncorrect, "");
					return;
				}
				mCompletion->Call(ValidateKeyPhraseWithS3JSONStatus::kSuccess, aesKey);
			}

			//
			std::string mKeyPhrase;
			std::string mKeystorePath;
			s3::GetS3ObjectResponsePtr mResponse;
			ValidateKeyPhraseWithS3JSONCompletionPtr mCompletion;
		};
		
		//
		void ValidateKeyPhraseWithS3JSON(const HermitPtr& h_,
										 const std::string& inKeyPhrase,
										 const s3bucket::S3BucketPtr& inS3Bucket,
										 const std::string& inS3BasePath,
										 const std::string& inKeyJSONFileName,
										 const ValidateKeyPhraseWithS3JSONCompletionPtr& inCompletion) {
			std::string basePathWithTrailingSlash;
			string::AddTrailingSlash(inS3BasePath, basePathWithTrailingSlash);
			std::string keyStorePath(basePathWithTrailingSlash);
			keyStorePath += inKeyJSONFileName;
			
			auto responseBlock = std::make_shared<s3::GetS3ObjectResponse>();
			auto completion = std::make_shared<CompletionBlock>(inKeyPhrase, keyStorePath, responseBlock, inCompletion);
			inS3Bucket->GetObject(h_, keyStorePath, responseBlock, completion);
		}
		
	} // namespace encoding_s3bucket
} // namespace hermit
