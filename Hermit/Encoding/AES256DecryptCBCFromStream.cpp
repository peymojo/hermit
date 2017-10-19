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
#include "Hermit/Foundation/Notification.h"
#include "AES256.h"
#include "AES256DecryptCBCFromStream.h"

namespace hermit {
	namespace encoding {
		
		namespace
		{
			//
			//
			class AES256StreamCalculator
			:
			public DecryptAES256PushDataFunction
			{
			public:
				//
				//
				AES256StreamCalculator(const AESKeySchedule& inKeySchedule,
									   const AESBlock& inPreviousBlock,
									   const AES256DecryptCallbackRef& inCallback)
				:
				mKeySchedule(inKeySchedule),
				mPreviousBlock(inPreviousBlock),
				mCallback(inCallback),
				mSuccess(false)
				{
				}
				
				//
				//
				virtual bool Function(const HermitPtr& h_,
									  const bool& inSuccess,
									  const DataBuffer& inData,
									  const bool& inEndOfStream) override {
					
					mSuccess = inSuccess;
					if (inSuccess)
					{
						if (inData.second)
						{
							mPlainText.append(inData.first, inData.second);
						}
						
						std::string plainText;
						
						size_t pos = 0;
						size_t bytesRemaining = mPlainText.size();
						while (bytesRemaining > 0)
						{
							AESBlock block;
							memset(&block, 0, sizeof(AESBlock));
							
							uint64_t bytes = 16;
							if (bytes > bytesRemaining) {
								NOTIFY_ERROR(h_, "AES256DecryptCBCFromStream: bytes > bytesRemaining.");
								return mCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
							}
							for (size_t x = 0; x < bytes; ++x)
							{
								block.bytes[x] = mPlainText[pos];
								++pos;
							}
							
							AESBlock result;
							memset(&result, 0, sizeof(AESBlock));
							Decode(block, mKeySchedule, result);
							
							for (uint64_t x = 0; x < 16; ++x)
							{
								result.bytes[x] ^= mPreviousBlock.bytes[x];
							}
							
							for (uint64_t x = 0; x < 16; ++x)
							{
								plainText += result.bytes[x];
							}
							
							mPreviousBlock = block;
							bytesRemaining -= bytes;
						}
						
						if (inEndOfStream) {
							//	pkcs7padding should always be present, and should be
							//	a value from 1 to 16 inclusive. The padding should be one of:
							//	01
							//	02 02
							//	03 03 03
							//	etc.
							size_t pkcs7padding = plainText[plainText.size() - 1];
							if (pkcs7padding > 16) {
								NOTIFY_ERROR(h_, "AES256DecryptCBCFromStream: pkcs7padding > 16.");
								return mCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
							}
							if (pkcs7padding == 0) {
								NOTIFY_ERROR(h_, "AES256DecryptCBCFromStream: pkcs7padding is 0.");
								return mCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
							}
							if (pkcs7padding > plainText.size()) {
								NOTIFY_ERROR(h_, "AES256DecryptCBCFromStream: pkcs7padding > plainText.size()");
								return mCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
							}
							for (size_t x = 1; x < pkcs7padding; ++x) {
								if (plainText[plainText.size() - 1 - x] != pkcs7padding) {
									NOTIFY_ERROR(h_, "AES256DecryptCBCFromStream: pkcs7padding bytes corrupted.");
									return mCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
								}
							}
							plainText.resize(plainText.size() - pkcs7padding);
						}
						else
						{
							mPlainText = mPlainText.substr(mPlainText.size() - bytesRemaining);
						}
						return mCallback.Call(kAES256DecryptCallbackStatus_Success,
											  DataBuffer(plainText.data(), plainText.size()),
											  inEndOfStream);
					}
					return true;
				}
				
				//
				//
				AESKeySchedule mKeySchedule;
				AESBlock mPreviousBlock;
				const AES256DecryptCallbackRef& mCallback;
				bool mSuccess;
				std::string mPlainText;
			};
			
		} // private namespace
		
		//
		//
		void AES256DecryptCBCFromStream(const HermitPtr& h_,
										const DecryptAES256GetDataFunctionRef& inFunction,
										const std::string& inKey,
										const std::string& inInputVector,
										const AES256DecryptCallbackRef& inCallback)
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
				inCallback.Call(kAES256DecryptCallbackStatus_Aborted, DataBuffer(), true);
			}
			else if (!calculator.mSuccess)
			{
				inCallback.Call(kAES256DecryptCallbackStatus_Error, DataBuffer(), true);
			}
		}
		
	} // namespace encoding
} // namespace hermit
