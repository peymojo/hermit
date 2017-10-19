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

#ifndef GenerateSecureRandomBytes_h
#define GenerateSecureRandomBytes_h

#include <string>
#include "Callback.h"
#include "Hermit.h"

namespace hermit {
	
	// success, randomBytes
	DEFINE_CALLBACK_2A(GenerateSecureRandomBytesCallback, bool,	std::string);
	
	//
	class GenerateSecureRandomBytesCallbackClass : public GenerateSecureRandomBytesCallback {
	public:
		//
		GenerateSecureRandomBytesCallbackClass() : mSuccess(false) {
		}
		
		//
		bool Function(const bool& success, const std::string& randomBytes) {
			mSuccess = success;
			if (success) {
				mValue = randomBytes;
			}
			return true;
		}
		
		//
		bool mSuccess;
		std::string mValue;
	};
	
	//
	void GenerateSecureRandomBytes(const HermitPtr& h_,
								   const uint64_t& inCount,
								   const GenerateSecureRandomBytesCallbackRef& inCallback);
	
} // namespace hermit

#endif
