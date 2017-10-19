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

#ifndef WithStringMapCallback_h
#define WithStringMapCallback_h

#include "Hermit/Foundation/Callback.h"
#include "StringMap.h"

namespace hermit {
namespace stringmap {

//
//
enum WithStringMapStatus
{
	kWithStringMapStatus_Unknown,
	kWithStringMapStatus_Success,
	kWithStringMapStatus_Canceled,
	kWithStringMapStatus_Error
};

//
//
DEFINE_CALLBACK_2A(
	WithStringMapCallback,
	WithStringMapStatus,			// inStatus
	StringMapPtr);					// inLookupTable
	
//
//
template <typename ManagedStringMapPtrT>
class WithStringMapCallbackClassT
	:
	public WithStringMapCallback
{
public:
	//
	//
	WithStringMapCallbackClassT()
		:
		mStatus(kWithStringMapStatus_Unknown)
	{
	}
	
	//
	//
	bool Function(
		const WithStringMapStatus& inStatus,
		const StringMapPtr& inStringMap)
	{
		mStatus = inStatus;
		if (inStatus == kWithStringMapStatus_Success)
		{
			mStringMap = inStringMap;
		}
		return true;
	}
	
	//
	//
	WithStringMapStatus mStatus;
	ManagedStringMapPtrT mStringMap;
};

} // namespace stringmap
} // namespace hermit

#endif
