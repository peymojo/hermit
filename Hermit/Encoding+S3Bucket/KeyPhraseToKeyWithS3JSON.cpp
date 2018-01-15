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
#include <string>
#include <vector>
#include "Hermit/Encoding/KeyPhraseToKeyWithValues.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/JSON/DataValueToJSON.h"
#include "Hermit/String/AddTrailingSlash.h"
#include "Hermit/Value/Value.h"
#include "KeyPhraseToKeyWithS3JSON.h"

namespace hermit {
	namespace encoding_s3bucket {
        namespace KeyPhraseToKeyWithS3JSON_Impl {
        
            //
            class PutObjectCompletion : public s3::PutS3ObjectCompletion {
            public:
                //
                PutObjectCompletion(const std::string& aesKey,
                                    const KeyPhraseToKeyWithS3JSONCompletionPtr& completion) :
                mAESKey(aesKey),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& version) {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kCanceled, "");
                        return;
                    }
                    if (result != s3::S3Result::kSuccess) {
                        NOTIFY_ERROR(h_, "PutObjectToS3Bucket failed.");
                        mCompletion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kError, "");
                        return;
                    }
                    mCompletion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kSuccess, mAESKey);
                }
                
                //
                std::string mAESKey;
                KeyPhraseToKeyWithS3JSONCompletionPtr mCompletion;
            };
            
        } // namespace KeyPhraseToKeyWithS3JSON_Impl
        using namespace KeyPhraseToKeyWithS3JSON_Impl;
        
		//
		void KeyPhraseToKeyWithS3JSON(const HermitPtr& h_,
                                      const std::string& keyPhrase,
                                      const s3bucket::S3BucketPtr& s3Bucket,
                                      const std::string& s3BasePath,
                                      const std::string& keyJSONName,
                                      const KeyPhraseToKeyWithS3JSONCompletionPtr& completion) {
			std::string basePathWithTrailingSlash;
			string::AddTrailingSlash(s3BasePath, basePathWithTrailingSlash);
			std::string keyStorePath(basePathWithTrailingSlash);
			keyStorePath += keyJSONName;
			
			std::string aesKey;
			value::ObjectValuePtr keyValues;
			auto status = encoding::KeyPhraseToKeyWithValues(h_, keyPhrase, aesKey, keyValues);
			if (status == encoding::KeyPhraseToKeyStatus::kCanceled) {
                completion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kCanceled, "");
				return;
			}
			if (status != encoding::KeyPhraseToKeyStatus::kSuccess) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithValues failed.");
                completion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kError, "");
				return;
			}
			
			std::string jsonString;
			json::DataValueToJSON(h_, keyValues, jsonString);
			if (jsonString.empty()) {
				NOTIFY_ERROR(h_, "DataValueToJSON failed.");
                completion->Call(h_, KeyPhraseToKeyWithS3JSONStatus::kError, "");
				return;
			}
            
            auto putCompletion = std::make_shared<PutObjectCompletion>(aesKey, completion);
			s3Bucket->PutObject(h_,
                                keyStorePath,
                                std::make_shared<SharedBuffer>(jsonString),
                                false,
                                putCompletion);
		}
		
	} // namespace encoding_s3bucket
} // namespace hermit
