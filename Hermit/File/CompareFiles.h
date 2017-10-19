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

#ifndef CompareFiles_h
#define CompareFiles_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "PreprocessFileFunction.h"

namespace hermit {
	namespace file {
		
		//
		enum class CompareFilesStatus {
			kUnknown,
			kSuccess,
			kCancel,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_1A(CompareFilesCompletion, CompareFilesStatus);

		//
		//	NOTE: inPreprocessFunction is NOT called for inFilePath1 & inFilePath2.
		//	It's called for all sub-files & nested sub-files if inFilePath1 & inFilePath2 are directories.
		//
		void CompareFiles(const HermitPtr& h_,
						  const FilePathPtr& inFilePath1,
						  const FilePathPtr& inFilePath2,
						  const bool& inIgnoreDates,
						  const bool& inIgnoreFinderInfo,
						  const PreprocessFileFunctionPtr& inPreprocessFunction,
						  const CompareFilesCompletionPtr& inCompletion);
		
	} // namespace file
} // namespace hermit

#endif
