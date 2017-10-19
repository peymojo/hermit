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
#include "GetFilePathComponents.h"
#include "FilePathToCocoaPathString.h"

namespace hermit {
	namespace file {
		
		namespace {
			//
			//	This used to swap in both directions... but this caused trouble
			//	for command-line arguments for (arguably deviant) files that actually
			//	had a : in the name of a node. So it now only swaps in one direction.
			//	If another failure case crops up that seems to need the reverse swapping,
			//	deeper consideration of how to handle all cases is needed.
			//
			std::string SubstitueColonsForSeparatorsInNodeName(const std::string& inPath) {
				if (inPath == "/") {
					return inPath;
				}
				std::string result;
				for (std::string::size_type n = 0; n < inPath.size(); ++n) {
					std::string::value_type ch = inPath[n];
					if (ch == '/') {
						ch = ':';
					}
					result.push_back(ch);
				}
				return result;
			}
			
		} // private namespace
		
		//
		void FilePathToCocoaPathString(const HermitPtr& h_, const FilePathPtr& filePath, std::string& outCocoaPathString) {
			if (filePath == nullptr) {
				NOTIFY_ERROR(h_, "FilePathToCocoaPathString: inFilePath is null.");
				outCocoaPathString = "";
				return;
			}
			
			std::vector<std::string> components;
			GetFilePathComponents(h_, filePath, components);
			if (components.empty()) {
				NOTIFY_ERROR(h_, "FilePathToCocoaPathString: GetFilePathComponents failed for path:", filePath);
			}
			
			std::string path;
			path.reserve(256);
			auto end = components.end();
			for (auto it = components.begin(); it != end; ++it) {
				std::string fixedComponent(SubstitueColonsForSeparatorsInNodeName(*it));
				if (!path.empty() && (path != "/")) {
					path += "/";
				}
				path += fixedComponent;
			}
			outCocoaPathString = path;
		}
		
	} // namespace file
} // namespace hermit

