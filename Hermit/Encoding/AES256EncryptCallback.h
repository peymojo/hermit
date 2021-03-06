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

#ifndef AES256EncryptCallback_h
#define AES256EncryptCallback_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"

namespace hermit {
	namespace encoding {
		
		//
		enum AES256EncryptCallbackStatus {
			kAES256EncryptCallbackStatus_Unknown,
			kAES256EncryptCallbackStatus_Success,
			kAES256EncryptCallbackStatus_Aborted,
			kAES256EncryptCallbackStatus_Error
		};
		
		//
		DEFINE_CALLBACK_3A(AES256EncryptCallback,
						   AES256EncryptCallbackStatus,		// inStatus
						   DataBuffer,						// inData
						   bool);							// inIsEndOfStream
		
	} // namespace encoding
} // namespace hermit

#endif
