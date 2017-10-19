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

#include "Base64.h"
#include "Base64ToBinary.h"

namespace hermit {
namespace encoding {

//
//
void Base64ToBinary(
	const DataBuffer& inBase64,
	const Base64ToBinaryCallbackRef& inCallback)
{
	std::string result;
	if (!Base64Decode(inBase64, result))
	{
		inCallback.Call(false, DataBuffer("", 0));
		return;
	}
	inCallback.Call(true, DataBuffer(result.data(), result.size()));
}
	
} // namespace encoding
} // namespace hermit
