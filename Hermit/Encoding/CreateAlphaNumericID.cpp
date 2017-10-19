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
		
		namespace {
			
			//
			class Buffer {
			public:
				//
				typedef size_t size_type;
				
				//
				Buffer() :
				mSize(0),
				mData(nullptr),
				mAllocatedBuf(nullptr) {
				}
				
				//
				~Buffer() {
					if (mAllocatedBuf != nullptr) {
						free(mAllocatedBuf);
					}
				}
				
				//
				void assign(const char* inData, size_t inDataSize) {
					if (inDataSize >= 256) {
						mAllocatedBuf = (char*)malloc(inDataSize + 1);
						if (mAllocatedBuf != nullptr) {
							memcpy(mAllocatedBuf, inData, inDataSize);
							mAllocatedBuf[inDataSize] = 0;
							mData = mAllocatedBuf;
						}
					}
					else {
						memcpy(mLocalBuf, inData, inDataSize);
						mLocalBuf[inDataSize] = 0;
						mData = mLocalBuf;
					}
					mSize = inDataSize;
				}
				
				//
				size_t mSize;
				char* mData;
				char mLocalBuf[256];
				char* mAllocatedBuf;
			};
			
			//
			bool ContainsUnwantedText(const char* inString) {
				static const char* sWords[] = {
					"arse",
					"ass",
					"bitch",
					"boob",
					"butt",
					"cock",
					"cunt",
					"damn",
					"dick",
					"fag",
					"fanny",
					"fart",
					"fuck",
					"hell",
					"homo",
					"nigger",
					"penis",
					"piss",
					"poop",
					"prick",
					"pussy",
					"rape",
					"sex",
					"shit",
					"slut",
					"tits",
					"turd",
					"twat",
					"whore" };
				const int kNumWords = 29;
				
				int trackers[kNumWords] = { 0 };
				const char* p = inString;
				while (*p) {
					for (int n = 0; n < kNumWords; ++n) {
						if (*p == sWords[n][trackers[n]]) {
							trackers[n]++;
							
							if (sWords[n][trackers[n]] == 0) {
								return true;
							}
						}
						else {
							trackers[n] = 0;
						}
					}
					
					p++;
				}
				return false;
			}
			
		} // private namespace
		
		//
		void CreateAlphaNumericID(const HermitPtr& h_, const uint32_t& inSize, std::string& outAlphaNumericID) {
			while (true) {
				GenerateSecureRandomBytesCallbackClass randomBytes;
				GenerateSecureRandomBytes(h_, inSize, randomBytes);
				if (!randomBytes.mSuccess || (randomBytes.mValue.empty())) {
					NOTIFY_ERROR(h_, "CreateAlphaNumericID: GenerateSecureRandomBytes failed, inSize:", inSize);
					outAlphaNumericID = "";
					return;
				}
				
				Buffer buf;
				buf.assign(randomBytes.mValue.data(), randomBytes.mValue.size());
				for (uint32_t n = 0; n < inSize; ++n) {
					//	always start with a letter...
					int mod = (n == 0) ? 26 : 36;
					int r = ((uint8_t)buf.mData[n]) % mod;
					if (r < 26) {
						buf.mData[n] = ('a' + r);
					}
					else {
						buf.mData[n] = ('0' + (r - 26));
					}
				}
				if (!ContainsUnwantedText(buf.mData)) {
					outAlphaNumericID = std::string(buf.mData, buf.mSize);
					break;
				}
			}
		}
		
	} // namespace encoding
} // namespace hermit
