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
#include "AES256EncryptCBCFromStream.h"

namespace hermit {
namespace encoding {

namespace
{
	//
	//
	class AES256StreamCalculator
		:
		public EncryptAES256PushDataFunction
	{	
	public:
		//
		//
		AES256StreamCalculator(
			const AESKeySchedule& inKeySchedule,
			const AESBlock& inPreviousBlock,
			const AES256EncryptCallbackRef& inCallback)
			:
			mKeySchedule(inKeySchedule),
			mPreviousBlock(inPreviousBlock),
			mCallback(inCallback),
			mSuccess(false)
		{
		}
				
		//
		//
		bool Function(
			const bool& inSuccess,
			const DataBuffer& inData,
			const bool& inEndOfStream)
		{
			mSuccess = inSuccess;
			if (inSuccess)
			{
				if (inData.second > 0)
				{
					mPlainText.append(inData.first, inData.second);
				}
				
				std::string cipherText;
			
				size_t pos = 0;
				size_t bytesRemaining = mPlainText.size();
				while ((bytesRemaining > 0) || inEndOfStream)
				{
					AESBlock block;
					memset(&block, 0, sizeof(AESBlock));
		
					uint64_t bytes = 16;
					if (bytes > bytesRemaining)
					{
						if (!inEndOfStream)
						{
							break;
						}					
						bytes = bytesRemaining;
					}
					for (size_t x = 0; x < bytes; ++x)
					{
						block.bytes[x] = mPlainText[pos];
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
						block.bytes[x] ^= mPreviousBlock.bytes[x];
					}
		
					AESBlock result;
					memset(&result, 0, sizeof(AESBlock));
					Encode(block, mKeySchedule, result);
		
					for (uint64_t x = 0; x < 16; ++x)
					{
						cipherText += result.bytes[x];
					}
		
					mPreviousBlock = result;

					if (bytes < 16)
					{
						break;
					}
		
					bytesRemaining -= bytes;
				}
				mPlainText = mPlainText.substr(mPlainText.size() - bytesRemaining);
				return mCallback.Call(kAES256EncryptCallbackStatus_Success,
									  DataBuffer(cipherText.data(), cipherText.size()),
									  inEndOfStream);
			}
			return true;
		}
		
		//
		//
		AESKeySchedule mKeySchedule;
		AESBlock mPreviousBlock;
		const AES256EncryptCallbackRef& mCallback;
		bool mSuccess;
		std::string mPlainText;
	};
	
} // private namespace

//
//
void AES256EncryptCBCFromStream(
	const EncryptAES256GetDataFunctionRef& inFunction,
	const std::string& inKey,
	const std::string& inInputVector,
	const AES256EncryptCallbackRef& inCallback)
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

	AES256StreamCalculator calculator(keySchedule, previousBlock, inCallback);
	if (!inFunction.Call(calculator))
	{
		inCallback.Call(kAES256EncryptCallbackStatus_Aborted, DataBuffer(), true);
	}
	else if (!calculator.mSuccess)
	{
		inCallback.Call(kAES256EncryptCallbackStatus_Error, DataBuffer(), true);
	}
}

} // namespace encoding
} // namespace hermit
