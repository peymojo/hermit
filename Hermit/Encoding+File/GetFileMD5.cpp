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

#include "GetFileMD5WithProgress.h"
#include "GetFileMD5.h"

namespace hermit {
	namespace encoding_file {
		
		//
		void GetFileMD5(const HermitPtr& h_,
						const file::FilePathPtr& filePath,
						const encoding::CalculateMD5CompletionPtr& completion) {
			GetFileMD5WithProgress(h_, filePath, completion);
		}
		
	} // namespace encoding_file
} // namespace hermit
