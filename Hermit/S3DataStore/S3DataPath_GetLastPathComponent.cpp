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

#include "S3DataPath.h"

namespace hermit {
	namespace s3datastore {
		
		//
		void S3DataPath::GetLastPathComponent(const HermitPtr& h_, std::string& outLastPathComponent) {
			if (mPath.empty()) {
				outLastPathComponent = "";
				return;
			}
			
			std::string path(mPath);
			if (path[path.size() - 1] == '/') {
				path = path.substr(0, path.size() - 1);
			}
			
			std::string::size_type slashPos = path.rfind("/");
			if (slashPos == std::string::npos) {
				outLastPathComponent = path;
				return;
			}
			std::string leaf(path.substr(slashPos + 1));
			outLastPathComponent = leaf;
		}
		
	} // namespace s3datastore
} // namespace hermit
