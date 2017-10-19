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

#include <string>
#include "BinaryToBase64Modified.h"

namespace hermit {
	namespace encoding {
		
		namespace {

			//
			static const char kIndexToBase64ModifiedTable[64] =
			{	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
				'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
				'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
				'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
				'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
				'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
				'w', 'x', 'y', 'z', '0', '1', '2', '3',
				'4', '5', '6', '7', '8', '9', '-', '_'	};
			
			//
			static void Encode3Bytes(const unsigned char* inBytes,
									 size_t inBytesRead,
									 char* inResult,
									 bool& outSkipByte2,
									 bool& outSkipByte3) {
				inResult[0] = kIndexToBase64ModifiedTable[inBytes[0] >> 2];
				inResult[1] = kIndexToBase64ModifiedTable[((inBytes[0] & 0x3) << 4) + (inBytesRead > 1 ? (inBytes[1] >> 4) : 0)];
				if (inBytesRead >= 2) {
					inResult[2] = kIndexToBase64ModifiedTable[((inBytes[1] & 0xf) << 2) + (inBytesRead > 2 ? (inBytes[2] >> 6) : 0)];
					outSkipByte2 = false;
				}
				else {
					outSkipByte2 = true;
				}
				if (inBytesRead >= 3) {
					inResult[3] = kIndexToBase64ModifiedTable[(inBytes[2] & 0x3f)];
					outSkipByte3 = false;
				}
				else {
					outSkipByte3 = true;
				}
			}
			
			//
			static void Base64ModifiedEncode(const char* inData, size_t inDataSize, std::string& outResult) {
				const unsigned char* p = reinterpret_cast<const unsigned char*>(inData);
				size_t bytesRemaining = inDataSize;
				while (bytesRemaining > 0) {
					char result[4];
					int bytesRead = 3;
					if (bytesRemaining < 3) {
						bytesRead = static_cast<int>(bytesRemaining);
					}
					bool skipByte2 = false;
					bool skipByte3 = false;
					Encode3Bytes(p, bytesRead, result, skipByte2, skipByte3);
					outResult.push_back(result[0]);
					outResult.push_back(result[1]);
					if (!skipByte2) {
						outResult.push_back(result[2]);
					}
					if (!skipByte3) {
						outResult.push_back(result[3]);
					}
					
					bytesRemaining -= bytesRead;
					p += bytesRead;
				}
			}
			
		} // private namespace
		
		//
		void BinaryToBase64Modified(const DataBuffer& inBinaryString, std::string& outBase64Modified) {
			Base64ModifiedEncode(inBinaryString.first, inBinaryString.second, outBase64Modified);
		}
		
	} // namespace encoding
} // namespace hermit
