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

#ifndef ParseXMLStream_h
#define ParseXMLStream_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"
#include "ParseXMLFunctions.h"

namespace hermit {
	namespace xml {
		
		//
		DEFINE_CALLBACK_2A(StreamInXMLDataCallback,
						   bool,							// inSuccess
						   DataBuffer);						// inData
		
		//
		DEFINE_CALLBACK_2A(StreamInXMLDataFunction,
						   uint64_t,						// inBytesRequested
						   StreamInXMLDataCallbackRef);		// inCallback
		
		//
		ParseXMLStatus ParseXMLStream(const HermitPtr& h_,
									  const StreamInXMLDataFunctionRef& inStreamFunction,
									  ParseXMLClient& client);
		
	} // namespace xml
} // namespace hermit

#endif