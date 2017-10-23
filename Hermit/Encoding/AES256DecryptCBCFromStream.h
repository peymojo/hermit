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

#ifndef AES256DecryptCBCFromStream_h
#define AES256DecryptCBCFromStream_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"
#include "AES256DecryptCallback.h"

namespace hermit {
	namespace encoding {
		
		//
		//
		DEFINE_CALLBACK_4A(DecryptAES256PushDataFunction,
						   HermitPtr,
						   bool,							// inSuccess
						   DataBuffer,						// inData
						   bool);							// inIsEndOfStream
		
		//
		//
		DEFINE_CALLBACK_1A(DecryptAES256GetDataFunction,
						   DecryptAES256PushDataFunctionRef);		// inPushFunction
		
		//
		//
		void AES256DecryptCBCFromStream(const HermitPtr& h_,
										const DecryptAES256GetDataFunctionRef& inFunction,
										const std::string& inKey,
										const std::string& inInputVector,
										const AES256DecryptCallbackRef& inCallback);
		
	} // namespace encoding
} // namespace hermit

#endif 