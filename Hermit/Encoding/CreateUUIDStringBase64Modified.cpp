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
#include "Hermit/Encoding/CreateUUIDString.h"
#include "Hermit/Encoding/HexToBase64Modified.h"
#include "CreateUUIDStringBase64Modified.h"

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//
			static std::string RemoveDashes(const std::string& inString) {
				std::string result;
				for (std::string::size_type n = 0; n < inString.size(); n++) {
					if (inString[n] != '-') {
						result += inString[n];
					}
				}
				return result;
			}
			
		} // private namespace
		
		//
		void CreateUUIDStringBase64Modified(std::string& outUUIDStringBase64Modified) {
			std::string uuidStr;
			CreateUUIDString(uuidStr);
			uuidStr = RemoveDashes(uuidStr);
			HexToBase64Modified(uuidStr, outUUIDStringBase64Modified);
		}
		
	} // namespace encoding
} // namespace hermit
