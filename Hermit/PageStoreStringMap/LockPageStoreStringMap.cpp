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
#include "PageStoreStringMap.h"
#include "LockPageStoreStringMap.h"

namespace hermit {
	namespace pagestorestringmap {
		
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
				LockCallback(const stringmap::LockStringMapCompletionFunctionPtr& inCompletionFunction)
				:
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void Call(const HermitPtr& h_, const TaskQueueLockStatus& inStatus) override {
					if (inStatus == kTaskQueueLockStatus_Cancel) {
						mCompletionFunction->Call(h_, stringmap::LockStringMapResult::kCanceled);
						return;
					}
					if (inStatus != kTaskQueueLockStatus_Success) {
						NOTIFY_ERROR(h_, "LockPageStoreStringMap: stringMap.Lock failed.");
						mCompletionFunction->Call(h_, stringmap::LockStringMapResult::kError);
						return;
					}
					mCompletionFunction->Call(h_, stringmap::LockStringMapResult::kSuccess);
				}
				
				//
				//
				stringmap::LockStringMapCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		//
		void LockPageStoreStringMap(const HermitPtr& h_,
									const stringmap::StringMapPtr& inStringMap,
									const stringmap::LockStringMapCompletionFunctionPtr& inCompletionFunction) {
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			TaskQueue& taskQueue = stringMap;
			auto lockCallback = std::make_shared<LockCallback>(inCompletionFunction);
			taskQueue.Lock(h_, lockCallback);
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
