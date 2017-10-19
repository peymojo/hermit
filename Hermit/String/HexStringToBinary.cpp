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
#include "HexStringToBinary.h"

namespace hermit {
	namespace string {
		
		namespace {

			//
			bool IsHex(char inChar) {
				if ((inChar >= '0') && (inChar <= '9')) {
					return true;
				}
				if ((inChar >= 'a') && (inChar <= 'f')) {
					return true;
				}
				if ((inChar >= 'A') && (inChar <= 'F')) {
					return true;
				}
				return false;
			}
			
			//
			uint8_t HexToBinary(char inChar) {
				if ((inChar >= '0') && (inChar <= '9')) {
					return inChar - '0';
				}
				if ((inChar >= 'a') && (inChar <= 'f')) {
					return 10 + (inChar - 'a');
				}
				if ((inChar >= 'A') && (inChar <= 'F')) {
					return 10 + (inChar - 'A');
				}
				return 0;
			}
			
			//
			uint8_t HexToBinary(char inChar1, char inChar2) {
				return (HexToBinary(inChar1) << 4) | HexToBinary(inChar2);
			}
			
			//
			std::string HexToBinary(const std::string& inHex) {
				std::string result;
				std::string::size_type pos = 0;
				while (true) {
					if (pos >= inHex.size()) {
						break;
					}
					std::string::value_type ch1 = inHex[pos];
					if (!IsHex(ch1)) {
						break;
					}
					++pos;
					if (pos >= inHex.size()) {
						break;
					}
					std::string::value_type ch2 = inHex[pos];
					if (!IsHex(ch2)) {
						break;
					}
					++pos;
					
					uint8_t value = HexToBinary(ch1, ch2);
					result.push_back(value);
				}
				return result;
			}
			
		} // private namespace
		
		//
		void HexStringToBinary(const std::string& hexString, std::string& outBinaryData) {
			outBinaryData = HexToBinary(hexString);
		}
		
	} // namespace string
} // namespace hermit

