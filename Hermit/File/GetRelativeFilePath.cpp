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
#include "Hermit/Foundation/Notification.h"
#include "CreateFilePathFromComponents.h"
#include "GetFilePathComponents.h"
#include "GetRelativeFilePath.h"

namespace hermit {
	namespace file {
		
		//
		void GetRelativeFilePath(const HermitPtr& h_,
								 const FilePathPtr& inFromPath,
								 const FilePathPtr& inToPath,
								 const GetRelativeFilePathCallbackRef& inCallback) {
			
			std::vector<std::string> fromComponents;
			GetFilePathComponents(h_, inFromPath, fromComponents);
			
			std::vector<std::string> toComponents;
			GetFilePathComponents(h_, inToPath, toComponents);
			
			auto fromEnd = fromComponents.end();
			auto fromIt = fromComponents.begin();
			auto toEnd = toComponents.end();
			auto toIt = toComponents.begin();
			while (true) {
				if (fromIt == fromEnd) {
					break;
				}
				if (toIt == toEnd) {
					break;
				}
				if (*fromIt != *toIt) {
					break;
				}
				++fromIt;
				++toIt;
			}
			
			std::vector<std::string> relativeComponents;
			while (fromIt != fromEnd) {
				relativeComponents.push_back("..");
				++fromIt;
			}
			while (toIt != toEnd) {
				relativeComponents.push_back(*toIt);
				++toIt;
			}
			
			FilePathPtr relativeFilePath;
			CreateFilePathFromComponents(relativeComponents, relativeFilePath);
			if (relativeFilePath == nullptr) {
				NOTIFY_ERROR(h_, "CreateFilePathFromComponents failed.");
				inCallback.Call(false, nullptr);
				return;
			}
			
			inCallback.Call(true, relativeFilePath);
		}
		
	} // namespace file
} // namespace hermit
