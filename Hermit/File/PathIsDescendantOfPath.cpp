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
#include <vector>
#include "GetFilePathComponents.h"
#include "PathIsDescendantOfPath.h"

namespace hermit {
	namespace file {
		
		//
		bool PathIsDescendantOfPath(const HermitPtr& h_, const FilePathPtr& candidateChildPath, const FilePathPtr& parentPath) {
			std::vector<std::string> childComponents;
			GetFilePathComponents(h_, candidateChildPath, childComponents);
			std::vector<std::string> parentComponents;
			GetFilePathComponents(h_, parentPath, parentComponents);
			auto childEnd = childComponents.end();
			auto childIt = childComponents.begin();
			auto parentEnd = parentComponents.end();
			auto parentIt = parentComponents.begin();
			while (true) {
				if (childIt == childEnd) {
					break;
				}
				if (parentIt == parentEnd) {
					break;
				}
				if (*childIt != *parentIt) {
					break;
				}
				++childIt;
				++parentIt;
			}
			return (parentIt == parentEnd);
		}
		
	} // namespace file
} // namespace hermit
