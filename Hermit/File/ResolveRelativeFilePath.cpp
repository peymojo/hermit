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
#include "AppendToFilePath.h"
#include "GetFilePathComponents.h"
#include "GetFilePathParent.h"
#include "ResolveRelativeFilePath.h"

namespace hermit {
	namespace file {
		
		//
		void ResolveRelativeFilePath(const HermitPtr& h_,
									 const FilePathPtr& inFilePath,
									 const FilePathPtr& inRelativePath,
									 FilePathPtr& outResolvedPath) {
			
			FilePathPtr resultPath(inFilePath);
			
			std::vector<std::string> fromComponents;
			GetFilePathComponents(h_, inRelativePath, fromComponents);
			if (fromComponents.empty()) {
				NOTIFY_ERROR(h_,
							 "ResolveRelativeFilePath: GetFilePathComponents returned empty components vector for path:",
							 inRelativePath);
				
				outResolvedPath = nullptr;
				return;
			}
			
			if (fromComponents[0] == "/") {
				NOTIFY_ERROR(h_,
							 "ResolveRelativeFilePath: inRelativePath appears to be an absolute path:",
							 inRelativePath);
				
				outResolvedPath = nullptr;
				return;
			}
			
			auto end = fromComponents.end();
			for (auto it = fromComponents.begin(); it != end; ++it) {
				std::string nextComponent(*it);
				if (nextComponent == "..") {
					FilePathPtr parentPath;
					GetFilePathParent(h_, resultPath, parentPath);
					if (parentPath == nullptr) {
						//	Can't go up any more levels from here.
						outResolvedPath = nullptr;
						return;
					}
					resultPath = parentPath;
				}
				else {
					FilePathPtr newPath;
					AppendToFilePath(h_, resultPath, nextComponent, newPath);
					if (newPath == nullptr) {
						NOTIFY_ERROR(h_,
									 "ResolveRelativeFilePath: AppendToFilePath failed for path:",
									 resultPath);
						
						NOTIFY_ERROR(h_, "-- and name:", nextComponent);
						
						outResolvedPath = nullptr;
						return;
					}
					resultPath = newPath;
				}
			}
			outResolvedPath = resultPath;
		}
		
	} // namespace file
} // namespace hermit
