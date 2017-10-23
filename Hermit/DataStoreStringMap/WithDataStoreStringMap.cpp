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

#include "Hermit/DataStorePageStore/WithDataStorePageStore.h"
#include "Hermit/PageStoreStringMap/WithPageStoreStringMap.h"
#include "Hermit/Foundation/Notification.h"
#include "WithDataStoreStringMap.h"

namespace hermit {
	namespace datastorestringmap {
		
		//
		stringmap::WithStringMapResult WithDataStoreStringMap(const HermitPtr& h_,
															  const datastore::DataStorePtr& inDataStore,
															  const datastore::DataPathPtr& inPath,
															  stringmap::StringMapPtr& outStringMap) {
			pagestore::WithPageStoreCallbackClassT<pagestore::PageStorePtr> pageStore;
			datastorepagestore::WithDataStorePageStore(inDataStore, inPath, pageStore);
			if (pageStore.mStatus == pagestore::kWithPageStoreStatus_Canceled) {
				return stringmap::WithStringMapResult::kCanceled;
			}
			if (pageStore.mStatus != pagestore::kWithPageStoreStatus_Success) {
				NOTIFY_ERROR(h_, "WithDataStoreStringMap: WithDataStorePageStore failed");
				return stringmap::WithStringMapResult::kError;
			}
			return pagestorestringmap::WithPageStoreStringMap(pageStore.mPageStore, outStringMap);
		}
		
	} // namespace datastorestringmap
} // namespace hermit
