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

#include "CompareMemory.h"

namespace hermit {
	
	//
	static uint64_t CompareMemory(const char* inData1, const char* inData2, uint64_t inSize) {
		uint64_t pos = 0;
		uint64_t sizeOfLong = sizeof(long);
		uint64_t slop = inSize - (inSize & ~(sizeOfLong - 1));
		uint64_t longs = inSize / sizeOfLong;
		const long* data1 = reinterpret_cast<const long*>(inData1);
		const long* data2 = reinterpret_cast<const long*>(inData2);
		for (uint64_t n = 0; n < longs; ++n) {
			if (*data1 != *data2) {
				const char* slop1 = reinterpret_cast<const char*>(data1);
				const char* slop2 = reinterpret_cast<const char*>(data2);
				for (uint64_t m = 0; m < sizeOfLong; ++m) {
					if (*slop1 != *slop2) {
						break;
					}
					++slop1;
					++slop2;
					++pos;
				}
				break;
			}
			++data1;
			++data2;
			pos += sizeOfLong;
		}
		if (pos == (inSize - slop)) {
			const char* slop1 = reinterpret_cast<const char*>(data1);
			const char* slop2 = reinterpret_cast<const char*>(data2);
			for (uint64_t n = 0; n < slop; ++n) {
				if (*slop1 != *slop2) {
					break;
				}
				++slop1;
				++slop2;
				++pos;
			}
		}
		return pos;
	}
	
	//
	void CompareMemory(const char* inData1,
					   const char* inData2,
					   const uint64_t& inSize,
					   const CompareMemoryCallbackRef& inCallback) {
		
		uint64_t equalBytes = CompareMemory(inData1, inData2, inSize);
		inCallback.Call((equalBytes == inSize), equalBytes);
	}
	
} // namespace hermit
