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

#include "Hermit/String/AddTrailingSlash.h"
#include "S3PathToDataPath.h"
#include "S3DataPath.h"

namespace hermit {
	namespace s3datastore {
		
		//
		void S3DataPath::AppendPathComponent(const HermitPtr& h_,
											 const std::string& inName,
											 const datastore::DataPathCallbackRef& inCallback) {
			
			std::string pathWithTrailingSlash;
			string::AddTrailingSlash(mPath, pathWithTrailingSlash);
			std::string newPath(pathWithTrailingSlash);
			newPath += inName;
			
			S3PathToDataPath(newPath, inCallback);
		}
		
	} // namespace s3datastore
} // namespace hermit
