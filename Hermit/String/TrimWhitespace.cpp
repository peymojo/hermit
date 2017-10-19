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
#include "TrimWhitespace.h"

namespace hermit {
	namespace string {
		
		namespace {
			
			//
			static bool IsWhiteSpace(std::string::value_type ch) {
				return ((ch == ' ') || (ch == '\t') || (ch == '\r') || (ch == '\n'));
			}
			
			//
			static std::string TrimWhitespace(const std::string& value) {
				if (value.empty()) {
					return std::string();
				}
				std::string::size_type startPos = 0;
				while ((startPos < value.size()) && IsWhiteSpace(value[startPos])) {
					++startPos;
				}
				std::string::size_type endPos = value.size();
				while ((endPos > 0) && (IsWhiteSpace(value[endPos - 1]))) {
					--endPos;
				}
				if (startPos >= endPos) {
					return std::string();
				}
				return value.substr(startPos, (endPos - startPos));
			}
			
		} // private namespace
		
		//
		//
		void TrimWhitespace(const std::string& value, std::string& outTrimmedValue) {
			outTrimmedValue = TrimWhitespace(value);
		}
		
	} // namespace string
} // namespace hermit
