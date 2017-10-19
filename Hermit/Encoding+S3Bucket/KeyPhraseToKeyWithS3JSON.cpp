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
		
		//
		void KeyPhraseToKeyWithS3JSON(const HermitPtr& h_,
									  const std::string& inKeyPhrase,
									  const s3bucket::S3BucketPtr& inS3Bucket,
									  const std::string& inS3BasePath,
									  const std::string& inKeyJSONName,
									  const KeyPhraseToKeyWithS3JSONCallbackRef& inCallback) {
			std::string basePathWithTrailingSlash;
			string::AddTrailingSlash(inS3BasePath, basePathWithTrailingSlash);
			std::string keyStorePath(basePathWithTrailingSlash);
			keyStorePath += inKeyJSONName;
			
			std::string aesKey;
			value::ObjectValuePtr keyValues;
			auto status = encoding::KeyPhraseToKeyWithValues(h_, inKeyPhrase, aesKey, keyValues);
			if (status == encoding::KeyPhraseToKeyStatus::kCanceled) {
				inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Canceled, "");
				return;
			}
			if (status != encoding::KeyPhraseToKeyStatus::kSuccess) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithS3Properties: KeyPhraseToKeyWithValues failed.");
				inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Error, "");
				return;
			}
			
			std::string jsonString;
			json::DataValueToJSON(h_, keyValues, jsonString);
			if (jsonString.empty()) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithS3Properties: DataValueToJSON failed.");
				inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Error, "");
				return;
			}
			
			s3::PutS3ObjectCallbackClass putCallback;
			inS3Bucket->PutObject(h_,
								  keyStorePath,
								  DataBuffer(jsonString.data(), jsonString.size()),
								  false,
								  putCallback);
			
			if (putCallback.mResult == s3::S3Result::kCanceled) {
				inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Canceled, "");
				return;
			}
			if (putCallback.mResult != s3::S3Result::kSuccess) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithS3Properties: PutObjectToS3Bucket failed.");
				inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Error, "");
				return;
			}
			inCallback.Call(kKeyPhraseToKeyWithS3JSONStatus_Success, aesKey);
		}
		
	} // namespace encoding_s3bucket
} // namespace hermit
