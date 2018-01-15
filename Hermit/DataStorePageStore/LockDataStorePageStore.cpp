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
		
		namespace {
			
			//
			//
			class LockCallback
			:
			public TaskQueueLockCallback
			{
			public:
				//
				//
				LockCallback(
							 const pagestore::LockPageStoreCompletionFunctionPtr& inCompletionFunction)
				:
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void Call(const HermitPtr& h_, const TaskQueueLockStatus& inStatus) override
				{
					if (inStatus == kTaskQueueLockStatus_Cancel)
					{
						mCompletionFunction->Call(h_, pagestore::kLockPageStoreStatus_Canceled);
						return;
					}
					if (inStatus != kTaskQueueLockStatus_Success)
					{
						NOTIFY_ERROR(h_, "LockDataStorePageStore: pageStore.Lock failed.");
						mCompletionFunction->Call(h_, pagestore::kLockPageStoreStatus_Canceled);
						return;
					}
					mCompletionFunction->Call(h_, pagestore::kLockPageStoreStatus_Success);
				}
				
				//
				//
				pagestore::LockPageStoreCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		void LockDataStorePageStore(const HermitPtr& h_,
									const pagestore::PageStorePtr& inPageStore,
									const pagestore::LockPageStoreCompletionFunctionPtr& inCompletionFunction)
		{
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			TaskQueue& taskQueue = pageStore;
			TaskQueueLockCallbackPtr lockCallback(new LockCallback(inCompletionFunction));
			taskQueue.Lock(h_, lockCallback);
		}
		
	} // namespace datastorepagestore
} // namespace hermit
