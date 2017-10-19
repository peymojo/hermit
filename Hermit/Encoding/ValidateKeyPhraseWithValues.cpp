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
#include "Hermit/Encoding/AES256DecryptCBC.h"
#include "Hermit/Encoding/Base64ToBinary.h"
#include "Hermit/Encoding/EncodeHMACSHA1PBKDF2.h"
#include "Hermit/Encoding/EncryptChallengeText.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/StringToUInt64.h"
#include "ValidateKeyPhraseWithValues.h"

namespace hermit {
	namespace encoding {
		
		//
		bool ValidateKeyPhraseWithValues(const HermitPtr& h_,
										 const std::string& inKeyPhrase,
										 const value::ObjectValuePtr& inValues,
										 std::string& outAESKey) {
		
			std::uint64_t iterations = 0;
			if (!value::GetUInt64Value(h_, inValues, "iterations", true, iterations)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: iterations value missing.");
				return false;
			}

			std::string saltBase64;
			if (!value::GetStringValue(h_, inValues, "salt", true, saltBase64)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: salt value missing.");
				return false;
			}
			
			std::string inputVectorBase64;
			if (!value::GetStringValue(h_, inValues, "inputvector", true, inputVectorBase64)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: inputvector value missing.");
				return false;
			}
			
			std::string challengeTextBase64;
			if (!value::GetStringValue(h_, inValues, "challengetext", true, challengeTextBase64)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: challengetext value missing.");
				return false;
			}
			
			Base64ToBinaryCallbackClass saltBase64Callback;
			Base64ToBinary(DataBuffer(saltBase64.data(), saltBase64.size()), saltBase64Callback);
			if (!saltBase64Callback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: base64 decode failed for salt.");
				return false;
			}
			std::string salt(saltBase64Callback.mValue);
			
			Base64ToBinaryCallbackClass inputVectorBase64Callback;
			Base64ToBinary(DataBuffer(inputVectorBase64.data(), inputVectorBase64.size()), inputVectorBase64Callback);
			if (!inputVectorBase64Callback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: base64 decode failed for input vector.");
				return false;
			}
			std::string inputVector(inputVectorBase64Callback.mValue);
			
			Base64ToBinaryCallbackClass challengeTextBase64Callback;
			Base64ToBinary(DataBuffer(challengeTextBase64.data(), challengeTextBase64.size()), challengeTextBase64Callback);
			if (!challengeTextBase64Callback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: base64 decode failed for encrypted challenge text.");
				return false;
			}
			std::string storedChallengeText(challengeTextBase64Callback.mValue);
			
			std::string aesKey;
			EncodeHMACSHA1PBKDF2(inKeyPhrase, salt, iterations, aesKey);
			
			std::string encryptedChallengeText;
			EncryptChallengeText(h_, aesKey, inputVector, encryptedChallengeText);
			if (encryptedChallengeText != storedChallengeText) {
				return false;
			}
			
			std::string encryptedKeyBase64;
			if (!value::GetStringValue(h_, inValues, "key", true, encryptedKeyBase64)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: key value missing.");
				return false;
			}
			
			Base64ToBinaryCallbackClass encryptedKeyBase64Callback;
			Base64ToBinary(DataBuffer(encryptedKeyBase64.data(), encryptedKeyBase64.size()), encryptedKeyBase64Callback);
			if (!encryptedKeyBase64Callback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: base64 decode failed for encrypted key.");
				return false;
			}
			std::string encryptedKey(encryptedKeyBase64Callback.mValue);
			
			std::string keyInputVectorBase64;
			if (!value::GetStringValue(h_, inValues, "keyinputvector", true, keyInputVectorBase64)) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: keyinputvector value missing.");
				return false;
			}
			
			Base64ToBinaryCallbackClass keyInputVectorBase64Callback;
			Base64ToBinary(DataBuffer(keyInputVectorBase64.data(), keyInputVectorBase64.size()), keyInputVectorBase64Callback);
			if (!keyInputVectorBase64Callback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: base64 decode failed for key input vector.");
				return false;
			}
			
			AES256DecryptCBCCallbackClass decryptKeyCallback;
			AES256DecryptCBC(h_,
							 DataBuffer(encryptedKey.data(), encryptedKey.size()),
							 aesKey,
							 keyInputVectorBase64Callback.mValue,
							 decryptKeyCallback);
			
			if (!decryptKeyCallback.mSuccess) {
				NOTIFY_ERROR(h_, "ValidateKeyPhraseWithProperties: AES256DecryptCBC failed.");
				return false;
			}
			
			outAESKey = decryptKeyCallback.mData;
			return true;
		}
		
	} // namespace encoding
} // namespace hermit
