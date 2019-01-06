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

#include <stdlib.h>
#include <string.h>
#include "Hermit/Foundation/GenerateSecureRandomBytes.h"
#include "Hermit/Foundation/Notification.h"
#include "CreateAlphaNumericID.h"

namespace hermit {
	namespace encoding {
		namespace CreateAlphaNumericID_Impl {
			
			//
			const char* kCharacterSet = "bcdfghjkmpqrtvwxy2346789";
			const int kCharacterSetSize = 24;
			const int kLettersCount = 17;
			
			//
			class Buffer {
			public:
				//
				Buffer() : mSize(0), mData(nullptr), mAllocatedBuf(nullptr) {
				}
				
				//
				~Buffer() {
					if (mAllocatedBuf != nullptr) {
						free(mAllocatedBuf);
					}
				}
				
				//
				void assign(const char* data, size_t dataSize) {
					if (dataSize >= 256) {
						mAllocatedBuf = (char*)malloc(dataSize + 1);
						if (mAllocatedBuf != nullptr) {
							memcpy(mAllocatedBuf, data, dataSize);
							mAllocatedBuf[dataSize] = 0;
							mData = mAllocatedBuf;
						}
					}
					else {
						memcpy(mLocalBuf, data, dataSize);
						mLocalBuf[dataSize] = 0;
						mData = mLocalBuf;
					}
					mSize = dataSize;
				}
				
				//
				size_t mSize;
				char* mData;
				char mLocalBuf[256];
				char* mAllocatedBuf;
			};
			
		} // namespace CreateAlphaNumericID_Impl
		using namespace CreateAlphaNumericID_Impl;
		
		//
		bool CreateAlphaNumericID(const HermitPtr& h_,
								  const uint32_t& size,
								  std::string& outAlphaNumericID) {
			std::string randomBytes;
			if (!GenerateSecureRandomBytes(h_, size, randomBytes)) {
				NOTIFY_ERROR(h_, "GenerateSecureRandomBytes failed, size:", size);
				return false;
			}
				
			Buffer buf;
			buf.assign(randomBytes.data(), randomBytes.size());
			for (uint32_t n = 0; n < size; ++n) {
				//	always start with a letter...
				int mod = (n == 0) ? kLettersCount : kCharacterSetSize;
				int r = ((uint8_t)buf.mData[n]) % mod;
				buf.mData[n] = kCharacterSet[r];
			}
			outAlphaNumericID = std::string(buf.mData, buf.mSize);
			return true;
		}
		
	} // namespace encoding
} // namespace hermit
