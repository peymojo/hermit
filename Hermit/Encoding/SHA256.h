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

#ifndef SHA256_H
#define SHA256_H

#include <cstdint>

namespace hermit {
namespace encoding {

//
struct SHA256State {
	std::uint32_t state[8];
	std::uint32_t total[2];
	std::uint32_t buflen;
	std::uint32_t buffer[32];
};

//
void SHA256Init(SHA256State& ioState);

//
void SHA224Init(SHA256State& ioState);

//
void SHA256ProcessBytes(SHA256State& ioState, const void* inData, std::uint64_t inDataSize);

//
void* SHA256Read(const SHA256State& inState, void* outResult);

//
void* SHA224Read(const SHA256State& inState, void* outResult);

//
void* SHA256Finish(SHA256State& ioState, void* outResult);

//
void* SHA224Finish(SHA256State& ioState, void* outResult);

//
void* CalculateSHA256(const char* inData, std::uint64_t inDataSize, void* outResult);

//
void* CalculateSHA224(const char* inData, std::uint64_t inDataSize, void* outResult);
	
} // namespace encoding
} // namespace hermit

#endif
