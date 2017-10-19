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
#include "StripCStyleComments.h"

namespace hermit {
	namespace string {
		
		//
		void StripCStyleComments(const std::string& line, std::string& outLineWithCommentsRemoved) {
			std::string result;
			std::string::size_type openPos = 0;
			std::string::size_type closePos = 0;
			while (true) {
				openPos = line.find("/*", openPos);
				if (openPos == std::string::npos) {
					result += line.substr(closePos);
					break;
				}
				result += line.substr(closePos, (openPos - closePos));
				closePos = line.find("*/", openPos + 1);
				if (closePos == std::string::npos) {
					break;
				}
				closePos += 2;
				openPos = closePos;
			}
			outLineWithCommentsRemoved = result;
		}
		
	} // namespace string
} // namespace hermit
