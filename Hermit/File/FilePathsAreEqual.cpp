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
#include "GetCanonicalFilePathString.h"
#include "FilePathsAreEqual.h"

namespace hermit {
	namespace file {
		
		//
		//
		void FilePathsAreEqual(const HermitPtr& h_,
							   const FilePathPtr& inPath1,
							   const FilePathPtr& inPath2,
							   const FilePathsAreEqualCallbackRef& inCallback) {
			
			std::string canonicalPath1;
			GetCanonicalFilePathString(h_, inPath1, canonicalPath1);
			
			std::string canonicalPath2;
			GetCanonicalFilePathString(h_, inPath2, canonicalPath2);
			
			inCallback.Call(true, (canonicalPath1 == canonicalPath2));
		}
		
	} // namespace file
} // namespace hermit
