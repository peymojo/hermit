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

#ifndef WithSQLiteStringMap_h
#define WithSQLiteStringMap_h

#include "Hermit/File/FilePath.h"
#include "Hermit/StringMap/StringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		
		//
		stringmap::WithStringMapResult WithSQLiteStringMap(const HermitPtr& h_,
														   const file::FilePathPtr& filePath,
														   stringmap::StringMapPtr& outStringMap);
		
	} // namespace pagestorestringmap
} // namespace hermit

#endif /* WithSQLiteStringMap_h */