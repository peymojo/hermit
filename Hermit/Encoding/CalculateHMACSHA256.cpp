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
#include "Hermit/Foundation/MemXOR.h"
#include "CalculateHMACSHA256.h"
#include "SHA256.h"

//
#define HMAC_IPAD 0x36
#define HMAC_OPAD 0x5c

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//
			static void CalculateHMACSHA256(const std::string& inKey, const std::string& inData, char* outResult) {
				char key[256];
				memset(key, 0, sizeof(key));
				size_t keySize = inKey.size();
				if (keySize > 64) {
					SHA256State state;
					SHA256Init(state);
					SHA256ProcessBytes(state, inKey.data(), inKey.size());
					SHA256Finish(state, key);
					keySize = 64;
				}
				else {
					memcpy(key, inKey.data(), inKey.size());
				}
				
				char block[64];
				memset(block, HMAC_IPAD, sizeof(block));
				MemXOR(block, key, keySize);
				
				SHA256State innerState;
				SHA256Init(innerState);
				SHA256ProcessBytes(innerState, block, sizeof(block));
				SHA256ProcessBytes(innerState, inData.data(), inData.size());
				char inner[256];
				memset(inner, 0, sizeof(inner));
				SHA256Finish(innerState, inner);
				
				memset(block, HMAC_OPAD, sizeof(block));
				MemXOR(block, key, keySize);
				
				SHA256State outerState;
				SHA256Init(outerState);
				SHA256ProcessBytes(outerState, block, sizeof(block));
				SHA256ProcessBytes(outerState, inner, 32);
				char outer[256];
				memset(outer, 0, sizeof(outer));
				SHA256Finish(outerState, outer);
				memcpy(outResult, outer, 32);
			}
			
		} // private namespace
		
		//
		void CalculateHMACSHA256(const std::string& inKey, const std::string& inData, std::string& outHMACSHA256) {
			char result[256];
			memset(result, 0, sizeof(result));
			CalculateHMACSHA256(inKey, inData, result);
			outHMACSHA256 = std::string(result, 32);
		}
		
	} // namespace encoding
} // namespace hermit
