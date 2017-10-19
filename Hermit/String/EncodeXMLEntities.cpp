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
#include "EncodeXMLEntities.h"

namespace hermit {
	namespace string {
		
		//
		void EncodeXMLEntities(const std::string& unencodedString, std::string& outResult) {
			std::string result;
			auto end = unencodedString.end();
			for (auto it = unencodedString.begin(); it != end; ++it) {
				char ch = *it;
				if (ch == '&') {
					result += "&amp;";
				}
				else if (ch == '<') {
					result += "&lt;";
				}
				else if (ch == '>') {
					result += "&gt;";
				}
				else if (ch == '\"') {
					result += "&quot;";
				}
				else if (ch == '\'') {
					result += "&apos;";
				}
				else if (ch < 32) {
					char buf[32];
					sprintf(buf, "&#%d;", (int)ch);
					result += buf;
				}
				else {
					result += ch;
				}
			}
			outResult = result;
		}
		
	} // namespace string
} // namespace hermit

