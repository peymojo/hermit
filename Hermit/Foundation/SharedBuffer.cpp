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

#include <memory>
#include <memory.h>
#include <stdlib.h>
#include "SharedBuffer.h"

namespace hermit {

//
SharedBuffer::SharedBuffer() : mData(nullptr), mDataSize(0) {
}
	
//
SharedBuffer::SharedBuffer(const char* inData, uint64_t inDataSize, bool inTakeOwnership)
	:
	mData(nullptr),
	mDataSize(0) {

	if (inTakeOwnership) {
		mData = inData;
		mDataSize = inDataSize;
	}
	else {
		Assign(inData, inDataSize);
	}
}
	
//
SharedBuffer::~SharedBuffer() {
	if (mData != nullptr) {
		free((void*)mData);
	}
}
	
//
void SharedBuffer::Assign(const char* inData, uint64_t inDataSize) {
	if (mData != nullptr) {
		free((void*)mData);
	}
	mData = nullptr;
	mDataSize = 0;

	if (inDataSize > 0) {
		mData = (const char*)malloc(inDataSize);
		if (mData == nullptr) {
			throw std::bad_alloc();
		}
		memcpy((void*)mData, inData, inDataSize);
		mDataSize = inDataSize;
	}
}

} // namespace hermit


