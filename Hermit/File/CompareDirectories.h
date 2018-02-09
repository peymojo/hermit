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

#ifndef CompareDirectories_h
#define CompareDirectories_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "CompareFiles.h"
#include "FilePath.h"
#include "HardLinkMap.h"
#include "PreprocessFileFunction.h"

namespace hermit {
	namespace file {
		
		//
		enum class CompareDirectoriesStatus {
			kUnknown,
			kSuccess,
			kCancel,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_1A(CompareDirectoriesCompletion, CompareDirectoriesStatus);
		
		//
		//	NOTE: preprocessFunction is NOT called for filePath1 & filePath2.
		//	It's called for all sub-files & nested sub-files of the directories.
		//
		void CompareDirectories(const HermitPtr& h_,
								const FilePathPtr& filePath1,
								const FilePathPtr& filePath2,
								const HardLinkMapPtr& hardLinkMap1,
								const HardLinkMapPtr& hardLinkMap2,
								const IgnoreDates& ignoreDates,
								const IgnoreFinderInfo& ignoreFinderInfo,
								const PreprocessFileFunctionPtr& preprocessFunction,
								const CompareDirectoriesCompletionPtr& completion);
		
	} // namespace file
} // namespace hermit

#endif
