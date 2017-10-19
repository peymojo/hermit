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
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "DecodeXMLEntities.h"

namespace hermit {
	namespace string {
		
		//
		void DecodeXMLEntities(const HermitPtr& h_, const std::string& encodedString, std::string& outResult) {
			std::string result;
			const char* p = encodedString.c_str();
			while (true) {
				const char ch = *p++;
				if (ch == 0) {
					break;
				}
				
				if (ch == '&') {
					if (strncmp(p, "amp;", 4) == 0) {
						result.push_back('&');
						p += 4;
					}
					else if (strncmp(p, "lt;", 3) == 0) {
						result.push_back('<');
						p += 3;
					}
					else if (strncmp(p, "gt;", 3) == 0) {
						result.push_back('>');
						p += 3;
					}
					else if (strncmp(p, "quot;", 5) == 0) {
						result.push_back('\"');
						p += 5;
					}
					else if (strncmp(p, "apos;", 5) == 0) {
						result.push_back('\'');
						p += 5;
					}
					else if (*p == '#') {
						++p;
						std::string numStr(p);
						std::string::size_type semiPos = numStr.find(';');
						if (semiPos == std::string::npos) {
							NOTIFY_ERROR(h_, "DecodeXMLEntities: Bad # entity sequence in string:", encodedString);
							result.push_back('#');
						}
						else {
							numStr = numStr.substr(0, semiPos);
							int val = atoi(numStr.c_str());
							if (val == 0) {
								NOTIFY_ERROR(h_, "DecodeXMLEntities: # entity value is 0 in string:", encodedString);
							}
							else if (val >= 32) {
								NOTIFY_ERROR(h_, "DecodeXMLEntities: # entity value is >= 32 in string:", encodedString);
							}
							else {
								result.push_back((char)val);
								p += (numStr.size() + 1);
							}
						}
					}
					else {
						NOTIFY_ERROR(h_, "DecodeXMLEntities: Unexpected entity sequence in string:", encodedString);
						result.push_back(ch);
					}
				}
				else {
					result.push_back(ch);
				}
			}
			outResult = result;
		}
		
	} // namespace string
} // namespace hermit
