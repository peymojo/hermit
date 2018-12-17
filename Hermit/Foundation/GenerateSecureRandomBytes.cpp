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

#include <Security/Security.h>
#include "Notification.h"
#include "GenerateSecureRandomBytes.h"

namespace hermit {
	
	//
	bool GenerateSecureRandomBytes(const HermitPtr& h_, const uint64_t& length, std::string& outResult) {
		uint8_t buf[256];
		uint8_t* allocatedBuf = nullptr;
		uint8_t* p = buf;
		if (length > 256) {
			allocatedBuf = (uint8_t*)malloc(length);
			if (allocatedBuf == nullptr) {
				NOTIFY_ERROR(h_, "GenerateSecureRandomBytes: malloc failed, size:", length);
				return false;
			}
			p = allocatedBuf;
		}
		
		bool success = false;
		int result = SecRandomCopyBytes(kSecRandomDefault, length, p);
		if (result != noErr) {
			NOTIFY_ERROR(h_, "GenerateSecureRandomBytes: SecRandomCopyBytes failed, errno:", errno);
		}
		else {
			outResult = std::string((const char*)p, length);
			success = true;
		}
		if (allocatedBuf != nullptr) {
			free(allocatedBuf);
		}
		return success;
	}
	
} // namespace hermit
