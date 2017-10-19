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
#include "Hermit/Encoding/BinaryToBase64Modified.h"
#include "Hermit/String/HexStringToBinary.h"
#include "HexToBase64Modified.h"

namespace hermit {
	namespace encoding {
		
		//
		void HexToBase64Modified(const std::string& inHexString, std::string& outBase64ModifiedString) {
			std::string binaryData;
			string::HexStringToBinary(inHexString, binaryData);
			BinaryToBase64Modified(DataBuffer(binaryData.data(), binaryData.size()), outBase64ModifiedString);
		}
		
	} // namespace encoding
} // namespace hermit
