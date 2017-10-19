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

#ifndef AES256EncryptCBCFromStream_h
#define AES256EncryptCBCFromStream_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "AES256EncryptCallback.h"

namespace hermit {
namespace encoding {

//
//
DEFINE_CALLBACK_3A(
	EncryptAES256PushDataFunction,
	bool,									// inSuccess
	DataBuffer,								// inData
	bool);									// inIsEndOfStream

//
//
DEFINE_CALLBACK_1A(
	EncryptAES256GetDataFunction,
	EncryptAES256PushDataFunctionRef);		// inPushFunction

//
//
void AES256EncryptCBCFromStream(
	const EncryptAES256GetDataFunctionRef& inFunction,
	const std::string& inKey,
	const std::string& inInputVector,
	const AES256EncryptCallbackRef& inCallback);

} // namespace encoding
} // namespace hermit

#endif 
