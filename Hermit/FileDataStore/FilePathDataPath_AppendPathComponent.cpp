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

#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/Foundation/Notification.h"
#include "FilePathToDataPath.h"
#include "FilePathDataPath.h"

namespace hermit {
	namespace filedatastore {
		
		//
		bool FilePathDataPath::AppendPathComponent(const HermitPtr& h_,
												   const std::string& name,
												   datastore::DataPathPtr& outDataPath) {
			
			file::FilePathPtr newPath;
			file::AppendToFilePath(h_, mFilePath, name, newPath);
			if (newPath == nullptr) {
				NOTIFY_ERROR(h_, "AppendToFilePath failed, parent path:", mFilePath, "name:", name);
				return false;
			}
			
			datastore::DataPathPtr dataPath;
			if (!FilePathToDataPath(h_, newPath, dataPath)) {
				NOTIFY_ERROR(h_, "FilePathToDataPath failed for path:", newPath);
				return false;
			}
			outDataPath = dataPath;
			return true;
		}
		
	} // namespace filedatastore
} // namespace hermit
