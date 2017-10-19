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

#ifndef CompareFinderInfo_h
#define CompareFinderInfo_h

#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum CompareFinderInfoStatus {
			kCompareFinderInfoStatus_Unknown,
			kCompareFinderInfoStatus_Match,
			kCompareFinderInfoStatus_FinderInfosDiffer,
			kCompareFinderInfoStatus_FolderPackageStatesDiffer,
			kCompareFinderInfoStatus_Error
		};
		
		//
		CompareFinderInfoStatus CompareFinderInfo(const HermitPtr& h_,
												  const FilePathPtr& inFilePath1,
												  const FilePathPtr& inFilePath2);
		
	} // namespace file
} // namespace hermit

#endif 
