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
#include "GetCommonPathParent.h"

namespace hermit {
	namespace string {
		
		//
		void GetCommonPathParent(const std::string& path1, const std::string& path2, std::string& outCommonParent) {
			std::string::size_type pos = 0;
			while (true) {
				if ((pos > path1.size()) || (pos > path2.size())) {
					break;
				}
				if (path1[pos] != path2[pos]) {
					break;
				}
				++pos;
			}
			std::string result;
			while ((pos > 0) && (path1[pos - 1] != '/')) {
				--pos;
			}
			if (pos > 0) {
				result = path1.substr(0, pos);
			}
			outCommonParent = result;
		}
		
	} // namespace string
} // namespace hermit
