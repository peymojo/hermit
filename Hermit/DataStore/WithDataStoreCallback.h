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

#ifndef WithDataStoreCallback_h
#define WithDataStoreCallback_h

#include "Hermit/Foundation/Callback.h"
#include "DataStore.h"

namespace hermit {
namespace datastore {

//
//
enum WithDataStoreCallbackStatus
{
	kWithDataStoreCallbackStatus_Unknown,
	kWithDataStoreCallbackStatus_Success,
	kWithDataStoreCallbackStatus_Error
};

//
//
DEFINE_CALLBACK_2A(
	WithDataStoreCallback,
	WithDataStoreCallbackStatus,		// inStatus
	DataStorePtr);						// inDataStore

//
//
template <typename ManagedDataStorePtrT>
class WithDataStoreCallbackClassT
	:
	public WithDataStoreCallback
{
public:
	//
	//
	WithDataStoreCallbackClassT()
	{
	}
	
	//
	//
	bool Function(
		const WithDataStoreCallbackStatus& inStatus,
		const DataStorePtr& inDataStore)
	{
		mStatus = inStatus;
		if (inStatus == kWithDataStoreCallbackStatus_Success)
		{
			mDataStore = inDataStore;
		}
		return true;
	}
		
	//
	//
	WithDataStoreCallbackStatus mStatus;
	ManagedDataStorePtrT mDataStore;
};

} // namespace datastore
} // namespace hermit

#endif
