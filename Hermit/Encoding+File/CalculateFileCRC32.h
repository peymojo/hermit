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

#ifndef CalculateFileCRC32_h
#define CalculateFileCRC32_h

#include "Hermit/File/FilePath.h"
#include "Hermit/Foundation/AsyncFunction.h"

namespace hermit {
	namespace encoding_file {
		
		//
		enum class CalculateFileCRC32Result {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		DEFINE_ASYNC_FUNCTION_3A(CalculateFileCRC32Completion, HermitPtr, CalculateFileCRC32Result, std::uint32_t);
		
		//
		void CalculateFileCRC32(const HermitPtr& h_,
								const file::FilePathPtr& filePath,
								const CalculateFileCRC32CompletionPtr& completion);
		
	} // namespace encoding_file
} // namespace hermit

#endif

