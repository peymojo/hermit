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
			
			//
			bool ContainsUnwantedText(const char* text) {
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
				const char* p = text;
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
		bool CreateAlphaNumericID(const HermitPtr& h_, const uint32_t& size, std::string& outAlphaNumericID) {
			while (true) {
				std::string randomBytes;
				if (!GenerateSecureRandomBytes(h_, size, randomBytes)) {
					NOTIFY_ERROR(h_, "GenerateSecureRandomBytes failed, size:", size);
					return false;
				}
				
				Buffer buf;
				buf.assign(randomBytes.data(), randomBytes.size());
				for (uint32_t n = 0; n < size; ++n) {
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
			return true;
		}
		
	} // namespace encoding
} // namespace hermit
