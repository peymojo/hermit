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

#include "BinaryStringToHex.h"

namespace hermit {
	namespace string {
		
		namespace {
			
			//
			static char ToHex(uint8_t inValue) {
				if (inValue < 10) {
					return '0' + inValue;
				}
				return 'a' + (inValue - 10);
			}
			
		} // private namespace
		
		//
		//
		void BinaryStringToHex(const std::string& binaryString, std::string& outHexString) {
			std::string result;
			std::string::size_type len = binaryString.size();
			for (std::string::size_type n = 0; n < len; ++n) {
				uint8_t value = binaryString[n];
				uint8_t char1 = (value >> 4);
				result.push_back(ToHex(char1));
				uint8_t char2 = (value & 0xf);
				result.push_back(ToHex(char2));
			}
			outHexString = result;
		}
		
	} // namespace string
} // namespace hermit
