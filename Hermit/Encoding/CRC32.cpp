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

namespace hermit {
namespace encoding {

namespace
{
	//
	//
	static uint32_t sCRC32Table[256];

	//
	//
	static bool sTableBuilt = false;

	//
	//
	void BuildCRC32Table()
	{
		for (int n = 0; n < 256; ++n)
		{
			uint32_t c = n;
			for (int x = 0; x < 8; ++x)
			{
				if (c & 1)
				{
					c = 0xedb88320L ^ (c >> 1);
				}
				else
				{
					c >>= 1;
				}
			}
			sCRC32Table[n] = c;
		}
		sTableBuilt = true;
	}
  
} // private namespace

//
//
uint32_t CRC32(
	const char* inData,
	uint64_t inDataSize)
{
	return UpdateCRC32(0xffffffffL, inData, inDataSize) ^ 0xffffffffL;
}

//
//
uint32_t UpdateCRC32(
	uint32_t inCRC32, 
	const char* inData,
	uint64_t inDataSize)
{
	uint32_t c = inCRC32;
	if (!sTableBuilt)
	{
		BuildCRC32Table();
	}
	const char* p = inData;
	for (uint64_t n = 0; n < inDataSize; ++n)
	{
		c = sCRC32Table[(c ^ *p++) & 0xff] ^ (c >> 8);
	}
	return c;
}
   
} // namespace encoding
} // namespace hermit
