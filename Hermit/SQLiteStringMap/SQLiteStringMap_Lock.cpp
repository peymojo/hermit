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
#include "Impl/SQLiteStringMapImpl.h"
#include "SQLiteStringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		namespace SQLiteStringMap_Lock_Impl {
			
			//
			class LockCallback : public LockTaskQueueCompletion {
			public:
				//
				LockCallback(const stringmap::LockStringMapCompletionFunctionPtr& completion) :
				mCompletion(completion) {
				}
				
				//
				void Call(const HermitPtr& h_, const LockTaskQueueResult& result) override {
					if (result == LockTaskQueueResult::kCancel) {
						mCompletion->Call(h_, stringmap::LockStringMapResult::kCanceled);
						return;
					}
					if (result != LockTaskQueueResult::kSuccess) {
						NOTIFY_ERROR(h_, "result != LockTaskQueueResult::kSuccess.");
						mCompletion->Call(h_, stringmap::LockStringMapResult::kError);
						return;
					}
					mCompletion->Call(h_, stringmap::LockStringMapResult::kSuccess);
				}
				
				//
				stringmap::LockStringMapCompletionFunctionPtr mCompletion;
			};
			
		} // namespace SQLiteStringMap_Lock_Impl
		using namespace SQLiteStringMap_Lock_Impl;
		
		void SQLiteStringMap::Lock(const HermitPtr& h_,
								   const stringmap::LockStringMapCompletionFunctionPtr& inCompletion) {
			auto callback = std::make_shared<LockCallback>(inCompletion);
			mImpl->Lock(h_, callback);
		}
		
	} // namespace sqlitestringmap
} // namespace hermit

