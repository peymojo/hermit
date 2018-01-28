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
		namespace LockPageStoreStringMap_Impl {
			
			//
			class LockCallback : public LockTaskQueueCompletion {
			public:
				//
				LockCallback(const stringmap::LockStringMapCompletionFunctionPtr& completion) : mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const LockTaskQueueResult& result) override {
					if (result == LockTaskQueueResult::kCancel) {
						mCompletion->Call(h_, stringmap::LockStringMapResult::kCanceled);
						return;
					}
					if (result != LockTaskQueueResult::kSuccess) {
						NOTIFY_ERROR(h_, "LockPageStoreStringMap: stringMap.Lock failed.");
						mCompletion->Call(h_, stringmap::LockStringMapResult::kError);
						return;
					}
					mCompletion->Call(h_, stringmap::LockStringMapResult::kSuccess);
				}
				
				//
				stringmap::LockStringMapCompletionFunctionPtr mCompletion;
			};
			
		} // namespace LockPageStoreStringMap_Impl
		using namespace LockPageStoreStringMap_Impl;
		
		//
		void LockPageStoreStringMap(const HermitPtr& h_,
									const stringmap::StringMapPtr& inStringMap,
									const stringmap::LockStringMapCompletionFunctionPtr& inCompletion) {
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			TaskQueue& taskQueue = stringMap;
			auto lockCallback = std::make_shared<LockCallback>(inCompletion);
			taskQueue.Lock(h_, lockCallback);
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
