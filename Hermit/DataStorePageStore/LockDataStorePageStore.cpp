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

#include "Hermit/Foundation/Notification.h"
#include "DataStorePageStore.h"
#include "LockDataStorePageStore.h"

namespace hermit {
	namespace datastorepagestore {
		namespace LockDataStorePageStore_Impl {
			
			//
			class LockCallback : public LockTaskQueueCompletion {
			public:
				//
				LockCallback(const pagestore::LockPageStoreCompletionFunctionPtr& completion) :
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const LockTaskQueueResult& result) override {
					if (result == LockTaskQueueResult::kCancel) {
						mCompletion->Call(h_, pagestore::kLockPageStoreStatus_Canceled);
						return;
					}
					if (result != LockTaskQueueResult::kSuccess) {
						NOTIFY_ERROR(h_, "LockDataStorePageStore: pageStore.Lock failed.");
						mCompletion->Call(h_, pagestore::kLockPageStoreStatus_Canceled);
						return;
					}
					mCompletion->Call(h_, pagestore::kLockPageStoreStatus_Success);
				}
				
				//
				pagestore::LockPageStoreCompletionFunctionPtr mCompletion;
			};
			
		} // namespace LockDataStorePageStore_Impl
		using namespace LockDataStorePageStore_Impl;
		
		//
		void LockDataStorePageStore(const HermitPtr& h_,
									const pagestore::PageStorePtr& inPageStore,
									const pagestore::LockPageStoreCompletionFunctionPtr& inCompletionFunction) {
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			TaskQueue& taskQueue = pageStore;
			auto lockCallback = std::make_shared<LockCallback>(inCompletionFunction);
			taskQueue.Lock(h_, lockCallback);
		}
		
	} // namespace datastorepagestore
} // namespace hermit
