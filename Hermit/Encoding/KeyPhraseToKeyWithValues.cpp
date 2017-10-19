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
#include "Hermit/Encoding/AES256EncryptCBC.h"
#include "Hermit/Encoding/BinaryToBase64.h"
#include "Hermit/Encoding/CreateInputVector.h"
#include "Hermit/Encoding/EncodeHMACSHA1PBKDF2.h"
#include "Hermit/Encoding/EncryptChallengeText.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/GenerateSecureRandomBytes.h"
#include "Hermit/Value/Value.h"
#include "KeyPhraseToKeyWithValues.h"

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//
			const uint64_t kPBKDF2Iterations = 256000;
			
			//
			typedef std::map<std::string, value::ValuePtr> ValueMap;
			
			//
			void KeyPhraseToAES256Key(const HermitPtr& h_,
									  const std::string& inKeyPhrase,
									  const int& inIterations,
									  std::string& outAESKey,
									  std::string& outSalt) {
				std::string salt;
				CreateInputVector(h_, 20, salt);
				EncodeHMACSHA1PBKDF2(inKeyPhrase, salt, inIterations, outAESKey);
				outSalt = salt;
			}
			
			//
			void EncryptChallengeText2(const HermitPtr& h_,
									   const std::string& inAESKey,
									   std::string& outEncryptedChallengeText,
									   std::string& outInputVector) {
				std::string inputVector;
				CreateInputVector(h_, 16, inputVector);
				EncryptChallengeText(h_, inAESKey, inputVector, outEncryptedChallengeText);
				outInputVector = inputVector;
			}
			
			//
			enum GenerateAndEncryptSecondaryKeyStatus {
				kGenerateAndEncryptSecondaryKeyStatus_Unknown,
				kGenerateAndEncryptSecondaryKeyStatus_Success,
				kGenerateAndEncryptSecondaryKeyStatus_Canceled,
				kGenerateAndEncryptSecondaryKeyStatus_Error
			};
			
			//
			GenerateAndEncryptSecondaryKeyStatus GenerateAndEncryptSecondaryKey(const HermitPtr& h_,
																				const std::string& inAESKey,
																				std::string& outSecondaryKey,
																				std::string& outEncryptedSecondaryKey,
																				std::string& outInputVector) {
				std::string inputVector;
				CreateInputVector(h_, 16, inputVector);
				GenerateSecureRandomBytesCallbackClass keyCallback;
				GenerateSecureRandomBytes(h_, 32, keyCallback);
				if (!keyCallback.mSuccess) {
					NOTIFY_ERROR(h_, "GenerateAndEncryptSecondaryKey: GenerateSecureRandomBytes failed.");
					return kGenerateAndEncryptSecondaryKeyStatus_Error;
				}
				std::string secondaryKey(keyCallback.mValue);
				
				AES256EncryptCBCCallbackClass encryptCallback;
				AES256EncryptCBC(h_,
								 DataBuffer(secondaryKey.data(), secondaryKey.size()),
								 inAESKey,
								 inputVector,
								 encryptCallback);
				
				if (encryptCallback.mStatus == kAES256EncryptCBC_Canceled) {
					return kGenerateAndEncryptSecondaryKeyStatus_Canceled;
				}
				if (encryptCallback.mStatus != kAES256EncryptCBC_Success) {
					NOTIFY_ERROR(h_, "GenerateAndEncryptSecondaryKey: AES256EncryptCBC failed.");
					return kGenerateAndEncryptSecondaryKeyStatus_Error;
				}
				
				outSecondaryKey = secondaryKey;
				outEncryptedSecondaryKey = encryptCallback.mValue;
				outInputVector = inputVector;
				return kGenerateAndEncryptSecondaryKeyStatus_Success;
			}
			
		} // private namespace
		
		//
		KeyPhraseToKeyStatus KeyPhraseToKeyWithValues(const HermitPtr& h_,
													  const std::string& keyPhrase,
													  std::string& outKey,
													  value::ObjectValuePtr& outValues) {
			std::string primaryKey;
			std::string salt;
			uint64_t iterations = kPBKDF2Iterations;
			KeyPhraseToAES256Key(h_, keyPhrase, kPBKDF2Iterations, primaryKey, salt);
			
			std::string encryptedChallengeText;
			std::string encryptedChallengeTextInputVector;
			EncryptChallengeText2(h_, primaryKey, encryptedChallengeText, encryptedChallengeTextInputVector);
			
			std::string saltBase64;
			BinaryToBase64(salt, saltBase64);
			
			std::string inputVectorBase64;
			BinaryToBase64(encryptedChallengeTextInputVector, inputVectorBase64);
			
			std::string encryptedChallengeTextBase64;
			BinaryToBase64(encryptedChallengeText, encryptedChallengeTextBase64);
			
			std::string secondaryKey;
			std::string encryptedSecondaryKey;
			std::string encryptedSecondaryKeyInputVector;
			auto encryptStatus = GenerateAndEncryptSecondaryKey(h_,
																primaryKey,
																secondaryKey,
																encryptedSecondaryKey,
																encryptedSecondaryKeyInputVector);
			
			if (encryptStatus == kGenerateAndEncryptSecondaryKeyStatus_Canceled) {
				return KeyPhraseToKeyStatus::kCanceled;
			}
			if (encryptStatus != kGenerateAndEncryptSecondaryKeyStatus_Success) {
				NOTIFY_ERROR(h_, "KeyPhraseToKeyWithValues: GenerateAndEncryptSecondaryKey failed.");
				return KeyPhraseToKeyStatus::kError;
			}
			
			std::string encryptedSecondaryKeyBase64;
			BinaryToBase64(encryptedSecondaryKey, encryptedSecondaryKeyBase64);
			
			std::string keyInputVectorBase64;
			BinaryToBase64(encryptedSecondaryKeyInputVector, keyInputVectorBase64);
			
			ValueMap keyAttributes;
			keyAttributes.insert(ValueMap::value_type("iterations", value::IntValue::New(iterations)));
			keyAttributes.insert(ValueMap::value_type("salt", value::StringValue::New(saltBase64)));
			keyAttributes.insert(ValueMap::value_type("inputvector", value::StringValue::New(inputVectorBase64)));
			keyAttributes.insert(ValueMap::value_type("challengetext", value::StringValue::New(encryptedChallengeTextBase64)));
			keyAttributes.insert(ValueMap::value_type("key", value::StringValue::New(encryptedSecondaryKeyBase64)));
			keyAttributes.insert(ValueMap::value_type("keyinputvector", value::StringValue::New(keyInputVectorBase64)));
			
			auto keyValues = std::make_shared<value::ObjectValueClassT<ValueMap>>(keyAttributes);
			outKey = secondaryKey;
			outValues = keyValues;
			return KeyPhraseToKeyStatus::kSuccess;
		}
		
	} // namespace encoding
} // namespace hermit
