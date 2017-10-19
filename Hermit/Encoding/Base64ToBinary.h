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

#ifndef Base64ToBinary_h
#define Base64ToBinary_h

#include <string>
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"

namespace hermit {
namespace encoding {

//
//
DEFINE_CALLBACK_2A(Base64ToBinaryCallback,
				   bool,						// inSuccess
				   DataBuffer);					// inData

//
//
class Base64ToBinaryCallbackClass
	:
	public Base64ToBinaryCallback
{
public:
	//
	//
	Base64ToBinaryCallbackClass()
		:
		mSuccess(false)
	{
	}
	
	//
	//
	bool Function(const bool& inSuccess, const DataBuffer& inData)
	{
		mSuccess = inSuccess;
		if (inSuccess)
		{
			mValue.assign(inData.first, inData.second);
		}
		return true;
	}
		
	//
	//
	bool mSuccess;
	std::string mValue;
};

//
//
void Base64ToBinary(
	const DataBuffer& inBase64,
	const Base64ToBinaryCallbackRef& inCallback);
	
} // namespace encoding
} // namespace hermit

#endif
