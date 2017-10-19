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

#ifndef WithPageStoreCallback_h
#define WithPageStoreCallback_h

#include "Hermit/Foundation/Callback.h"
#include "PageStore.h"

namespace hermit {
namespace pagestore {


//
//
enum WithPageStoreStatus
{
	kWithPageStoreStatus_Unknown,
	kWithPageStoreStatus_Success,
	kWithPageStoreStatus_Canceled,
	kWithPageStoreStatus_Error
};

//
//
DEFINE_CALLBACK_2A(
	WithPageStoreCallback,
	WithPageStoreStatus,			// inStatus
	PageStorePtr);					// inLookupTable
	
//
//
template <typename ManagedPageStorePtrT>
class WithPageStoreCallbackClassT
	:
	public WithPageStoreCallback
{
public:
	//
	//
	WithPageStoreCallbackClassT()
		:
		mStatus(kWithPageStoreStatus_Unknown)
	{
	}
	
	//
	//
	bool Function(
		const WithPageStoreStatus& inStatus,
		const PageStorePtr& inPageStore)
	{
		mStatus = inStatus;
		if (inStatus == kWithPageStoreStatus_Success)
		{
			mPageStore = inPageStore;
		}
		return true;
	}
	
	//
	//
	WithPageStoreStatus mStatus;
	ManagedPageStorePtrT mPageStore;
};

} // namespace pagestore
} // namespace hermit

#endif 
