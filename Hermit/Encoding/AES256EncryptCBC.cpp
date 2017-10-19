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

#include <memory.h>
#include <string>
#include "AES256.h"
#include "AES256EncryptCBC.h"

namespace hermit {
	namespace encoding {
		
		//
		//
		void AES256EncryptCBC(const HermitPtr& h_,
							  const DataBuffer& inPlainText,
							  const std::string& inKey,
							  const std::string& inInputVector,
							  const AES256EncryptCBCCallbackRef& inCallback)
		{
			AESKey key;
			memset(&key, 0, sizeof(AESKey));
			int len = (int)inKey.size();
			if (len > 32)
			{
				len = 32;
			}
			for (int n = 0; n < len; ++n)
			{
				key.bytes[n] = inKey[n];
			}
			AESKeySchedule keySchedule;
			KeyExpansion(key, keySchedule);
			
			//	for (int n = 0; n < 60; ++n)
			//	{
			//		std::cout << "  " << n << " => " << keySchedule.words[n] << "\n";
			//	}
			
			uint64_t textSize = inPlainText.second;
			uint64_t blocks = (textSize / 16) + 1;
			
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
			
			std::string cipherText;
			size_t pos = 0;
			for (uint64_t n = 0; n < blocks; ++n)
			{
				if (((n + 1) % 100000) == 0)
				{
					if (CHECK_FOR_ABORT(h_))
					{
						inCallback.Call(kAES256EncryptCBC_Canceled, DataBuffer());
						return;
					}
				}
				
				AESBlock block;
				memset(&block, 0, sizeof(AESBlock));
				
				uint64_t bytes = 16;
				if (bytes > (textSize - pos))
				{
					bytes = textSize - pos;
				}
				for (uint64_t x = 0; x < bytes; ++x)
				{
					block.bytes[x] = inPlainText.first[pos];
					++pos;
				}
				if (bytes < 16)
				{
					uint8_t pkcs7padding = 16 - bytes;
					for (uint64_t x = 0; x < pkcs7padding; ++x)
					{
						block.bytes[bytes + x] = pkcs7padding;
					}
				}
				
				for (uint64_t x = 0; x < 16; x++)
				{
					block.bytes[x] ^= previousBlock.bytes[x];
				}
				
				AESBlock result;
				memset(&result, 0, sizeof(AESBlock));
				Encode(block, keySchedule, result);
				
				for (uint64_t x = 0; x < 16; ++x)
				{
					cipherText += result.bytes[x];
				}
				
				previousBlock = result;
			}
			inCallback.Call(kAES256EncryptCBC_Success, DataBuffer(cipherText.data(), cipherText.size()));
		}
		
	} // namespace encoding
} // namespace hermit
