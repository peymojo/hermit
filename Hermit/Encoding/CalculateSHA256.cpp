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

#include "CalculateSHA256.h"
#include "SHA256.h"

namespace hermit {
	namespace encoding {
		
		//
		void CalculateSHA256(const std::string& data, std::string& outSHA256) {
			char buf[32] = { 0 };
			CalculateSHA256(data.data(), data.size(), buf);
			outSHA256 = std::string(buf, 32);
		}
		
	} // namespace encoding
} // namespace hermit
