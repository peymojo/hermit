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
#include "WriteFileData.h"
#include "WriteUTF8File.h"

namespace hermit {
	namespace file {
		
		namespace {

			//
			class WriteLineCallback : public WriteUTF8FileLineCallback {
			public:
				//
				bool Function(const std::string& inLine) {
					mString += inLine;
					mString += "\n";
					return true;
				}
				
				//
				std::string mString;
			};
			
			//
			std::string CreateFileData(const WriteUTF8FileLineFunctionRef& inWriteLineFunction) {
				WriteLineCallback callback;
				inWriteLineFunction.Call(callback);
				return callback.mString;
			}
			
		} // private namespace
		
		//
		WriteFileDataResult WriteUTF8File(const HermitPtr& h_,
										  const FilePathPtr& inFilePath,
										  const WriteUTF8FileLineFunctionRef& inWriteLineFunction) {
			std::string data(CreateFileData(inWriteLineFunction));
			return WriteFileData(h_, inFilePath, DataBuffer(data.data(), data.size()));
		}
		
	} // namespace file
} // namespace hermit
