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

#ifndef GetPrimaryMACAddress_h
#define GetPrimaryMACAddress_h

#include <string>
#include "Callback.h"
#include "Hermit.h"

namespace hermit {
	
//
enum GetPrimaryMacAddressResult {
	kGetPrimaryMacAddressResult_Unknown,
	kGetPrimaryMacAddressResult_Success,
	kGetPrimaryMacAddressResult_Error
};
	
//
DEFINE_CALLBACK_2A(GetPrimareyMACAddressCallback,
				   GetPrimaryMacAddressResult,
				   std::string);

//
template <typename StringType>
class GetPrimareyMACAddressCallbackClassT :
	public GetPrimareyMACAddressCallback {
public:
	//
	GetPrimareyMACAddressCallbackClassT() :
		mResult(kGetPrimaryMacAddressResult_Unknown) {
	}
	
	//
	bool Function(const GetPrimaryMacAddressResult& inResult,
				  const std::string& inMACAddress) {

		mResult = inResult;
		if (inResult == kGetPrimaryMacAddressResult_Success) {
			mMACAddress = inMACAddress;
		}
		return true;
	}
	
	//
	GetPrimaryMacAddressResult mResult;
	StringType mMACAddress;
};

//
void GetPrimaryMACAddress(const HermitPtr& h_, const GetPrimareyMACAddressCallbackRef& callback);
	
} // namespace hermit

#endif
