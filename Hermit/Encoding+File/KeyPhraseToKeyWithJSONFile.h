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

#ifndef KeyPhraseToKeyWithJSONFile_h
#define KeyPhraseToKeyWithJSONFile_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/File/FilePath.h"

namespace hermit {
	namespace encoding_file {
		
		//
		enum class KeyPhraseToKeyWithJSONFileResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_4A(KeyPhraseToKeyWithJSONFileCompletion,
								 HermitPtr,
								 KeyPhraseToKeyWithJSONFileResult,
								 std::string,								// inAESKey
								 file::FilePathPtr);						// inKeyJSONFilePath
		//
		void KeyPhraseToKeyWithJSONFile(const HermitPtr& h_,
										const std::string& inKeyPhrase,
										const file::FilePathPtr& inParentDirectory,
										const std::string& inKeyJSONFileName,
										const KeyPhraseToKeyWithJSONFileCompletionPtr& inCompletion);
		
	} // namespace encoding_file
} // namespace hermit

#endif
