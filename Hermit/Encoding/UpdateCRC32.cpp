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

#include "CRC32.h"
#include "UpdateCRC32.h"

namespace hermit {
	namespace encoding {
		
		std::uint32_t UpdateCRC32(const std::uint32_t& inCRC32, const DataBuffer& inData) {
			return UpdateCRC32(inCRC32, inData.first, inData.second);
		}
		
	} // namespace encoding
} // namespace hermit
