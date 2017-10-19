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

#ifndef WriteUTF8File_h
#define WriteUTF8File_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "WriteFileData.h"

namespace hermit {
	namespace file {
		
		//
		DEFINE_CALLBACK_1A(WriteUTF8FileLineCallback,
						   std::string);						//	inLine
		
		//
		DEFINE_CALLBACK_1A(WriteUTF8FileLineFunction,
						   WriteUTF8FileLineCallbackRef);		//	inCallback
		
		//
		WriteFileDataResult WriteUTF8File(const HermitPtr& h_,
										  const FilePathPtr& inFilePath,
										  const WriteUTF8FileLineFunctionRef& inWriteLineFunction);
		
	} // namespace file
} // namespace hermit

#endif

