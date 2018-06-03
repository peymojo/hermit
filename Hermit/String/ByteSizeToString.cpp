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
#include "DoubleToString.h"
#include "UInt64ToString.h"
#include "ByteSizeToString.h"

namespace hermit {
	namespace string {
		
		//
		static std::string ByteSizeToString(const HermitPtr& h_, uint64_t inSize) {
			static const uint64_t oneKilobyte = 1024;
			static const uint64_t oneMegabyte = oneKilobyte * 1024;
			static const uint64_t oneGigabyte = oneMegabyte * 1024;
			static const uint64_t oneTerabyte = oneGigabyte * 1024;
			
			std::string result;
			uint64_t size = inSize;
			if (size > oneTerabyte) {
				double teraBytes = ((inSize * 100) / oneTerabyte) / 100.0;
				std::string str;
				DoubleToString(teraBytes, 2, str);
				str += " TB";
				result = str;
			}
			else if (size > oneGigabyte) {
				double gigaBytes = ((inSize * 100) / oneGigabyte) / 100.0;
				std::string str;
				DoubleToString(gigaBytes, 2, str);
				str += " GB";
				result = str;
			}
			else if (size > oneMegabyte) {
				double megaBytes = ((inSize * 100) / oneMegabyte) / 100.0;
				std::string str;
				DoubleToString(megaBytes, 2, str);
				str += " MB";
				result = str;
			}
			else if (size > oneKilobyte) {
				double kiloBytes = ((inSize * 100) / oneKilobyte) / 100.0;
				std::string str;
				DoubleToString(kiloBytes, 2, str);
				str += " KB";
				result = str;
			}
			else {
				std::string str;
				UInt64ToString(h_, size, str);
				str += " bytes";
				result = str;
			}
			return result;
		}
		
		//
		//
		void ByteSizeToString(const HermitPtr& h_, const uint64_t& bytes, std::string& outString) {
			outString = ByteSizeToString(h_, bytes);
		}
		
	} // namespace string
} // namespace hermit
