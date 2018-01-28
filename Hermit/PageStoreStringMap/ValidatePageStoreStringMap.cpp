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
#include "ValidatePageStoreStringMap.h"

namespace hermit {
	namespace pagestorestringmap {
		namespace ValidatePageStoreStringMap_Impl {
			
			//
			class ValidatePageStoreCompletion : public pagestore::ValidatePageStoreCompletionFunction {
			public:
				//
				ValidatePageStoreCompletion(const stringmap::ValidateStringMapCompletionFunctionPtr& completion) :
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const pagestore::ValidatePageStoreResult& inResult) override {
					if (inResult == pagestore::ValidatePageStoreResult::kCanceled) {
						mCompletion->Call(h_, stringmap::ValidateStringMapResult::kCanceled);
					}
					if (inResult != pagestore::ValidatePageStoreResult::kSuccess) {
						NOTIFY_ERROR(h_, "ValidatePageStoreStringMap: ValidatePageStore failed.");
						mCompletion->Call(h_, stringmap::ValidateStringMapResult::kError);
						return;
					}
					mCompletion->Call(h_, stringmap::ValidateStringMapResult::kSuccess);
				}
				
				//
				stringmap::ValidateStringMapCompletionFunctionPtr mCompletion;
			};
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const stringmap::StringMapPtr& stringMap,
					 const stringmap::ValidateStringMapCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					auto completion = std::make_shared<ValidatePageStoreCompletion>(mCompletion);
					stringMap.mPageStore->Validate(h_, completion);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::ValidateStringMapCompletionFunctionPtr mCompletion;
			};
			
			//
			class CompletionProxy : public stringmap::ValidateStringMapCompletionFunction {
			public:
				//
				CompletionProxy(const stringmap::StringMapPtr& inStringMap,
								const stringmap::ValidateStringMapCompletionFunctionPtr& inCompletionFunction) :
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const stringmap::ValidateStringMapResult& inResult) override {
					mCompletionFunction->Call(h_, inResult);
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					stringMap.TaskComplete();
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::ValidateStringMapCompletionFunctionPtr mCompletionFunction;
			};
			
		} // namespace ValidatePageStoreStringMap_Impl
		using namespace ValidatePageStoreStringMap_Impl;
		
		//
		void ValidatePageStoreStringMap(const HermitPtr& h_,
										const stringmap::StringMapPtr& stringMap,
										const stringmap::ValidateStringMapCompletionFunctionPtr& completion) {
			PageStoreStringMap& pageStoreStringMap = static_cast<PageStoreStringMap&>(*stringMap);
			auto completionProxy = std::make_shared<CompletionProxy>(stringMap, completion);
			auto task = std::make_shared<Task>(stringMap, completionProxy);
			if (!pageStoreStringMap.QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "stringMap.QueueTask failed");
				completion->Call(h_, stringmap::ValidateStringMapResult::kError);
			}
		}
		
	} // namespace pagestorestringmap
} // namespace hermit

