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

#ifndef FilePath_h
#define FilePath_h

#include <iostream>
#include <memory>
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace file {
		
		//
		struct FilePath {
			virtual ~FilePath() = default;
		};
		typedef std::shared_ptr<FilePath> FilePathPtr;
		
		//
		void StreamOut(const HermitPtr& h_, std::ostream& strm, const FilePathPtr& filePath);
		
		//
		template <class... B>
		void StreamOut(const HermitPtr& h_, std::ostream& strm, const FilePathPtr& filePath, B... rest) {
			StreamOut(h_, strm, filePath);
			strm << " ";
			StreamOut(h_, strm, rest...);
		}
		
	} // namespace file
} // namespace hermit

#endif