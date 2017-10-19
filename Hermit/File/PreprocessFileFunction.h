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

#ifndef PreprocessFileFunction_h
#define PreprocessFileFunction_h

#include <memory>
#include <string>
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		enum class PreprocessFileInstruction {
			kUnknown,
			kContinue,
			kSkip,
			kError
		};
		
		//
		class PreprocessFileFunction {
		public:
			//
			virtual ~PreprocessFileFunction() = default;
			//
			virtual PreprocessFileInstruction Preprocess(const HermitPtr& h_, const FilePathPtr& parent, const std::string& itemName) = 0;
		};
		typedef std::shared_ptr<PreprocessFileFunction> PreprocessFileFunctionPtr;
		
	} // namespace file
} // namespace hermit

#endif
