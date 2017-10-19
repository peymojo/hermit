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
#include "AES256.h"
#include "AES256DecryptCBC.h"

namespace hermit {
	namespace encoding {
		
		//
		//
		void AES256DecryptCBC(const HermitPtr& h_,
							  const DataBuffer& inCipherText,
							  const std::string& inKey,
							  const std::string& inInputVector,
							  const AES256DecryptCBCCallbackRef& inCallback)
		{
			if (inCipherText.second == 0)
			{
				NOTIFY_ERROR(h_, "AES256DecryptCBC: no cipherText?");
				inCallback.Call(false, DataBuffer());
				return;
			}
			
			AESKey key;
			memset(&key, 0, sizeof(AESKey));
			uint64_t len = inKey.size();
			if (len > 32)
			{
				len = 32;
			}
			for (uint64_t n = 0; n < len; ++n)
			{
				key.bytes[n] = inKey[n];
			}
			AESKeySchedule keySchedule;
			KeyExpansion(key, keySchedule);
			
			size_t textSize = (size_t)inCipherText.second;
			uint64_t blocks = textSize / 16;
			if (textSize % 16 != 0)
			{
				blocks++;
			}
			
			AESBlock previousBlock;
			memset(&previousBlock, 0, sizeof(AESBlock));
			
			uint64_t inputVectorSize = inInputVector.size();
			uint64_t bytes = 16;
			if (bytes > inputVectorSize)
			{
				bytes = inputVectorSize;
			}
			for (uint64_t x = 0; x < bytes; ++x)
			{
				previousBlock.bytes[x] = inInputVector[x];
			}
			
			std::string plainText;
			size_t pos = 0;
			for (uint64_t n = 0; n < blocks; ++n)
			{
				AESBlock block;
				memset(&block, 0, sizeof(AESBlock));
				
				uint64_t bytes = 16;
				if (bytes > (textSize - pos))
				{
					bytes = textSize - pos;
				}
				for (uint64_t x = 0; x < bytes; ++x)
				{
					block.bytes[x] = inCipherText.first[pos];
					++pos;
				}
				
				AESBlock result;
				memset(&result, 0, sizeof(AESBlock));
				Decode(block, keySchedule, result);
				
				for (uint64_t x = 0; x < 16; ++x)
				{
					result.bytes[x] ^= previousBlock.bytes[x];
				}
				
				for (uint64_t x = 0; x < 16; ++x)
				{
					plainText += result.bytes[x];
				}
				
				previousBlock = block;
			}
			
			//	Plaintext is not allowed to be empty at this stage because there
			//	should always be padding.
			if (plainText.empty())
			{
				NOTIFY_ERROR(h_, "AES256DecryptCBC: plainText is empty.");
				inCallback.Call(false, DataBuffer());
				return;
			}
			
			//	pkcs7padding should always be present, and should be
			//	a value from 1 to 16 inclusive. The padding should be one of:
			//	01
			//	02 02
			//	03 03 03
			//	etc.
			uint8_t pkcs7padding = plainText[plainText.size() - 1];
			if (pkcs7padding > 16)
			{
				NOTIFY_ERROR(h_, "AES256DecryptCBC: pkcs7padding > 16.");
				inCallback.Call(false, DataBuffer());
				return;
			}
			if (pkcs7padding == 0)
			{
				NOTIFY_ERROR(h_, "AES256DecryptCBC: pkcs7padding is 0.");
				inCallback.Call(false, DataBuffer());
				return;
			}
			if (pkcs7padding > plainText.size())
			{
				NOTIFY_ERROR(h_, "AES256DecryptCBC: pkcs7padding > plainText.size()");
				inCallback.Call(false, DataBuffer());
				return;
			}
			for (size_t x = 1; x < pkcs7padding; ++x)
			{
				if (plainText[plainText.size() - 1 - x] != pkcs7padding)
				{
					NOTIFY_ERROR(h_, "AES256DecryptCBC: pkcs7padding bytes corrupted.");
					inCallback.Call(false, DataBuffer());
					return;
				}
			}
			plainText.resize(plainText.size() - pkcs7padding);
			
			inCallback.Call(true, DataBuffer(plainText.data(), plainText.size()));
		}	
		
	} // namespace encoding
} // namespace hermit
