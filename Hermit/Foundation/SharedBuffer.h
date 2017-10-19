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

#ifndef SharedBuffer_h
#define SharedBuffer_h

namespace hermit {

//
class SharedBuffer {
public:
	//
	SharedBuffer();
	
	//
	SharedBuffer(const char* inData, uint64_t inDataSize, bool inTakeOwnership = false);
	
	//
	template <typename StringT>
	SharedBuffer(const StringT& inString) : mData(nullptr), mDataSize(0) {
		Assign(inString.data(), inString.size());
	}
	
	//
	~SharedBuffer();
	
	//
	void Assign(const char* inData, uint64_t inDataSize);
	
	//
	const char* Data() const {
		return mData;
	}
	
	//
	size_t Size() const {
		return mDataSize;
	}
	
private:
	//
	const char* mData;
	size_t mDataSize;
};

//
typedef std::shared_ptr<SharedBuffer> SharedBufferPtr;

} // namespace hermit

#endif
