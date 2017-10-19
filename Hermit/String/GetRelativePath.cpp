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
#include "GetRelativePath.h"

namespace hermit {
	namespace string {
		
		//
		void GetRelativePath(const std::string& anchorPath, const std::string& targetPath, std::string& outRelativePath) {
			std::string commonRoot;
			GetCommonPathParent(anchorPath, targetPath, commonRoot);
			
			bool noCommonPath = commonRoot.empty() || ((commonRoot.size() == 1) && (commonRoot[0] == '/'));
			
			std::string result;
			if (!noCommonPath) {
				std::string localAnchorPath(anchorPath.substr(commonRoot.size()));
				std::string targetAnchorPath(targetPath.substr(commonRoot.size()));
				
				while (true) {
					std::string::size_type slashPos = localAnchorPath.find("/");
					if (slashPos == std::string::npos) {
						break;
					}
					result += "../";
					localAnchorPath = localAnchorPath.substr(slashPos + 1);
				}
				result += targetAnchorPath;
			}
			outRelativePath = result;
		}
		
	} // namespace string
} // namespace hermit
