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

#include <sstream>
#include <vector>
#include "Base64.h"

namespace hermit {
	namespace encoding {
		
		//
		const char kIndexToBase64Table[64] =
		{	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
			'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
			'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
			'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
			'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
			'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
			'w', 'x', 'y', 'z', '0', '1', '2', '3',
			'4', '5', '6', '7', '8', '9', '+', '/'	};
		
		//
		static void Encode3Bytes(const unsigned char* inBytes, size_t inBytesRead, char* inResult) {
			inResult[0] = kIndexToBase64Table[inBytes[0] >> 2];
			inResult[1] = kIndexToBase64Table[((inBytes[0] & 0x3) << 4) + (inBytesRead > 1 ? (inBytes[1] >> 4) : 0)];
			if (inBytesRead >= 2) {
				inResult[2] = kIndexToBase64Table[((inBytes[1] & 0xf) << 2) + (inBytesRead > 2 ? (inBytes[2] >> 6) : 0)];
			}
			else {
				inResult[2] = '=';
			}
			if (inBytesRead >= 3) {
				inResult[3] = kIndexToBase64Table[(inBytes[2] & 0x3f)];
			}
			else {
				inResult[3] = '=';
			}
		}
		
		//
		void Base64Encode(const char* inData, size_t inDataSize, std::string& outBase64) {
			if (inDataSize == 0) {
				//	empty data is represented in base64 by... empty string
				outBase64.clear();
				return;
			}
			
			std::string base64;
			const unsigned char* p = reinterpret_cast<const unsigned char*>(inData);
			size_t bytesRemaining = inDataSize;
			while (bytesRemaining > 0) {
				char result[4];
				int bytesRead = 3;
				if (bytesRemaining < 3) {
					bytesRead = static_cast<int>(bytesRemaining);
				}
				Encode3Bytes(p, bytesRead, result);
				base64.push_back(result[0]);
				base64.push_back(result[1]);
				base64.push_back(result[2]);
				base64.push_back(result[3]);
				
				bytesRemaining -= bytesRead;
				p += bytesRead;
			}
			outBase64 = base64;
		}
		
		//
		static int Decode1Byte(char inByte) {
			if ((inByte >= 'A') && (inByte <= 'Z')) {
				return inByte - 'A';
			}
			
			if ((inByte >= 'a') && (inByte <= 'z')) {
				return 26 + (inByte - 'a');
			}
			
			if ((inByte >= '0') && (inByte <= '9')) {
				return 52 + (inByte - '0');
			}
			
			if (inByte == '+') {
				return 62;
			}
			
			if (inByte == '/') {
				return 63;
			}
			
			if (inByte == '=') {
				return -1;
			}
			
			return -2;
		}
		
		//
		static int Decode4Bytes(const char* inBytes, unsigned char* inResult) {
			int decodedBytes[4];
			for (int n = 0; n < 4; n++) {
				decodedBytes[n] = Decode1Byte(inBytes[n]);
				if (decodedBytes[n] == -2) {
					return -2;
				}
			}
			
			inResult[0] = (decodedBytes[0] << 2) + (decodedBytes[1] >> 4);
			if (decodedBytes[2] == -1) {
				return 1;
			}
			
			inResult[1] = ((decodedBytes[1] & 0xf) << 4) + (decodedBytes[2] >> 2);
			if (decodedBytes[3] == -1) {
				return 2;
			}
			
			inResult[2] = ((decodedBytes[2] & 0x3) << 6) + (decodedBytes[3]);
			return 3;
		}
		
		//
		bool Base64Decode(const DataBuffer& base64Data, std::string& outResult) {
			if (base64Data.second == 0) {
				//	empty string is vaild base64 and represents... empty data
				outResult.clear();
				return true;
			}
			
			std::istringstream stream(std::string(base64Data.first, base64Data.second));
			stream.setf(std::ios_base::skipws);
			std::vector<char> buffer;
			while (true) {
				char inBytes[4];
				for (int n = 0; n < 4; ++n) {
					inBytes[n] = 0;
				}
				
				int bytesRead = 0;
				while (stream.good() && (bytesRead < 4)) {
					//  Use operator>> to automatically skip whitespace.
					if (stream >> inBytes[bytesRead]) {
						++bytesRead;
					}
				}
				if (bytesRead == 0) {
					break;
				}
				
				unsigned char outBytes[3];
				int bytesDecoded = Decode4Bytes(inBytes, outBytes);
				if (bytesDecoded == -2) {
					return false;
				}
				for (int m = 0; m < bytesDecoded; ++m) {
					buffer.push_back(outBytes[m]);
				}
			}
			if (buffer.empty()) {
				return false;
			}
			outResult = std::string(&buffer.at(0), buffer.size());
			return true;
		}
		
	} // namespace encoding
} // namespace hermit
