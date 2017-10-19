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
#include "CalculateHMACSHA1.h"
#include "SHA1.h"

//
#define SHA1_IPAD 0x36
#define SHA1_OPAD 0x5c

namespace hermit {
	namespace encoding {
		
		//
		static std::string CalculateHMACSHA1(const std::string& key, const std::string& data) {
			std::string keyToUse(key);
			if (keyToUse.size() > 64) {
				SHA1Encoder keyHash;
				keyHash.ProcessBytes(keyToUse.data(), keyToUse.size());
				keyToUse = keyHash.GetResult();
			}
			
			char block[64];
			memset(block, SHA1_IPAD, sizeof(block));
			MemXOR(block, keyToUse.data(), keyToUse.size());
			
			SHA1Encoder inner;
			inner.ProcessBytes(block, sizeof(block));
			inner.ProcessBytes(data.data(), data.size());
			std::string innerHash(inner.GetResult());
			
			memset(block, SHA1_OPAD, sizeof(block));
			MemXOR(block, keyToUse.data(), keyToUse.size());
			
			SHA1Encoder outer;
			outer.ProcessBytes(block, sizeof(block));
			outer.ProcessBytes(innerHash.data(), innerHash.size());
			return outer.GetResult();
		}
		
		//
		void CalculateHMACSHA1(const std::string& key, const std::string& data, std::string& outHMACSHA1) {
			outHMACSHA1 = CalculateHMACSHA1(key, data);
		}
		
	} // namespace encoding
} // namespace hermit
