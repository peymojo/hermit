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
#include <vector>
#include "Hermit/Encoding/CalculateHMACSHA1.h"
#include "EncodeHMACSHA1PBKDF2.h"

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//
			static std::string Encode(const std::string& inPassphrase,
									  const std::string& inSalt,
									  const uint32_t& inBlockNumber,
									  const uint64_t& inIterations) {
				std::vector<unsigned char> buf1(inSalt.size() + 4);
				memcpy(&buf1.at(0), inSalt.data(), inSalt.size());
				buf1[inSalt.size()] = (unsigned char)((inBlockNumber >> 24) & 0xff);
				buf1[inSalt.size() + 1] = (unsigned char)((inBlockNumber >> 16) & 0xff);
				buf1[inSalt.size() + 2] = (unsigned char)((inBlockNumber >> 8) & 0xff);
				buf1[inSalt.size() + 3] = (unsigned char)(inBlockNumber & 0xff);
				
				std::string hmacSHA1;
				CalculateHMACSHA1(inPassphrase, std::string((const char*)&buf1.at(0), buf1.size()), hmacSHA1);
				
				unsigned char buf2[20];
				memcpy(buf2, hmacSHA1.data(), 20);
				unsigned char result[20];
				memcpy(result, buf2, 20);
				
				for (uint64_t i = 1; i < inIterations; ++i) {
					std::string innerHMACSHA1;
					CalculateHMACSHA1(inPassphrase, std::string((const char*)buf2, 20), innerHMACSHA1);
					
					memcpy(buf2, innerHMACSHA1.data(), 20);
					for (int j = 0; j < 20; ++j) {
						result[j] ^= buf2[j];
					}
				}
				return std::string((const char*)result, 20);
			}
			
		} // private namespace
		
		//
		void EncodeHMACSHA1PBKDF2(const std::string& inPassphrase,
								  const std::string& inSalt,
								  const uint64_t& inIterations,
								  std::string& outResult) {
			std::string block1(Encode(inPassphrase, inSalt, 1, inIterations));
			std::string block2(Encode(inPassphrase, inSalt, 2, inIterations));
			std::string result(block1);
			result += block2;
			outResult = result;
		}
		
	} // namespace encoding
} // namespace hermit
