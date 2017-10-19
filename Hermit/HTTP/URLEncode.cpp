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
#include "URLEncode.h"

namespace hermit {
	namespace http {
		
		namespace {
			
			//
			static char ToHex(uint8_t inValue) {
				if (inValue < 10) {
					return '0' + inValue;
				}
				return 'A' + (inValue - 10);
			}
			
		} // private namespace
		
		//
		void URLEncode(const std::string& stringToEncode, const bool& encodeSlashes, std::string& outURLEncodedString) {
			std::string result;
			for (std::string::size_type i = 0; i < stringToEncode.size(); ++i) {
				uint8_t ch = stringToEncode[i];
				if (ch == ' ') {
					result += "+";
				}
				else {
					bool legalChar = ((ch >= 'A') && (ch <= 'Z')) ||
					((ch >= 'a') && (ch <= 'z')) ||
					((ch >= '0') && (ch <= '9')) ||
					(ch == '_') ||
					(ch == '-') ||
					(ch == '~') ||
					(ch == '.');
					
					if ((ch == '/') && !encodeSlashes) {
						legalChar = true;
					}
					
					if (!legalChar) {
						result += "%";
						uint8_t char1 = (ch >> 4);
						result.push_back(ToHex(char1));
						uint8_t char2 = (ch & 0xf);
						result.push_back(ToHex(char2));
					}
					else {
						result += ch;
					}
				}
			}
			outURLEncodedString = result;
		}
		
	} // namespace http
} // namespace hermit
