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
#include "StripRedundantWhitespace.h"

namespace hermit {
	namespace string {
		
		namespace {
			
			//
			static bool IsWhiteSpace(std::string::value_type inChar) {
				return ((inChar == ' ') || (inChar == '\t') || (inChar == '\r') || (inChar == '\n'));
			}
			
		} // private namespace
		
		//
		void StripRedundantWhitespace(const std::string& text, std::string& outTextWithoutRedundantWhitespace) {
			std::string result;
			bool lastCharWasWhitespace = false;
			std::string::size_type len = text.size();
			for (std::string::size_type n = 0; n < len; ++n) {
				std::string::value_type ch = text[n];
				if (IsWhiteSpace(ch)) {
					if (!lastCharWasWhitespace) {
						result.push_back(' ');
					}
					lastCharWasWhitespace = true;
				}
				else {
					result.push_back(ch);
					lastCharWasWhitespace = false;
				}
			}
			outTextWithoutRedundantWhitespace = result;
		}
		
	} // namespace string
} // namespace hermit


