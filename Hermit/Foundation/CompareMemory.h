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

#ifndef CompareMemory_h
#define CompareMemory_h

#include <cstdint>
#include "Callback.h"

namespace hermit {
	
	//
	DEFINE_CALLBACK_2A(CompareMemoryCallback,
					   bool,						// inMatch
					   std::uint64_t);				// inBytesThatMatch
	
	//
	class CompareMemoryCallbackClass : public CompareMemoryCallback {
	public:
		//
		CompareMemoryCallbackClass()
		:
		mMatch(false),
		mBytesThatMatch(0) {
		}
		
		//
		bool Function(const bool& inMatch, const std::uint64_t& inBytesThatMatch) {
			mMatch = inMatch;
			mBytesThatMatch = inBytesThatMatch;
			return true;
		}
		
		//
		bool mMatch;
		std::uint64_t mBytesThatMatch;
	};
	
	//
	void CompareMemory(const char* inData1,
					   const char* inData2,
					   const std::uint64_t& inSize,
					   const CompareMemoryCallbackRef& inCallback);
	
} // namespace hermit

#endif
