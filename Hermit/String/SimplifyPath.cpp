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
#include "Hermit/Foundation/Notification.h"
#include "AddTrailingSlash.h"
#include "SimplifyPath.h"

namespace hermit {
	namespace string {
		
		//
		bool SimplifyPath(const HermitPtr& h_, const std::string& path, const std::string& basePath, std::string& outUpdatedPath) {
			if (path.empty()) {
				NOTIFY_ERROR(h_, "SimplifyPath: Empty path provided.");
				return false;
			}
			if (path[0] == '/') {
				// not a relative path
				outUpdatedPath = path;
				return true;
			}
			if (basePath.empty()) {
				NOTIFY_ERROR(h_, "SimplifyPath: Empty base path provided; Can't resolve relative path:", path);
				return false;
			}

			std::string updatedBasePath;
			AddTrailingSlash(basePath, updatedBasePath);
			std::string result(updatedBasePath + path);
			
			while (true) {
				std::string::size_type pos = result.find("/.");
				if (pos == std::string::npos) {
					break;
				}
					
				if (pos == result.size() - 2) {
					result = result.substr(0, pos);
				}
				else if (result[pos + 2] == '/') {
					result = result.substr(0, pos) + result.substr(pos + 2);
				}
				else if (result[pos + 2] == '.') {
					if (pos == 0) {
						break;
					}
					if (pos == result.size() - 3) {
						std::string::size_type prevPos = result.rfind("/", pos - 1);
						if (prevPos == std::string::npos) {
							break;
						}
						result = result.substr(0, prevPos);
					}
					else if (result[pos + 3] == '/') {
						std::string::size_type prevPos = result.rfind("/", pos - 1);
						if (prevPos == std::string::npos) {
							break;
						}
						result = result.substr(0, prevPos) + result.substr(pos + 3);
					}
					else {
						break;
					}
				}
				else {
					break;
				}
			}
			outUpdatedPath = result;
			return true;
		}
		
	} // namespace string
} // namespace hermit

