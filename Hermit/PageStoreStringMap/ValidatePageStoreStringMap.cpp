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
#include "Hermit/Foundation/ParamBlock.h"
#include "PageStoreStringMap.h"
#include "ValidatePageStoreStringMap.h"

namespace hermit {
	namespace pagestorestringmap {
		
		namespace {
			
			//
			//
			DEFINE_PARAMBLOCK_3(Params,
								HermitPtr, h_,
								stringmap::StringMapPtr, stringMap,
								stringmap::ValidateStringMapCompletionFunctionPtr, completion);
			
			//
			class ValidatePageStoreCompletion : public pagestore::ValidatePageStoreCompletionFunction {
			public:
				//
				ValidatePageStoreCompletion(const Params& inParams) : mParams(inParams) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const pagestore::ValidatePageStoreResult& inResult) override {
					if (inResult == pagestore::ValidatePageStoreResult::kCanceled) {
						mParams.completion->Call(h_, stringmap::ValidateStringMapResult::kCanceled);
					}
					if (inResult != pagestore::ValidatePageStoreResult::kSuccess) {
						NOTIFY_ERROR(mParams.h_, "ValidatePageStoreStringMap: ValidatePageStore failed.");
						mParams.completion->Call(h_, stringmap::ValidateStringMapResult::kError);
						return;
					}
					
					mParams.completion->Call(h_, stringmap::ValidateStringMapResult::kSuccess);
				}
				
				//
				Params mParams;
			};
			
			//
			//
			class Task
			:
			public AsyncTask
			{
			public:
				//
				//
				Task(const Params& inParams)
				:
				mParams(inParams)
				{
				}
				
				//
				//
				virtual void PerformTask(const int32_t& inTaskID)
				{
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mParams.stringMap);
					
					pagestore::ValidatePageStoreCompletionFunctionPtr completion(new ValidatePageStoreCompletion(mParams));
					stringMap.mPageStore->Validate(mParams.h_, completion);
				}
				
				//
				//
				Params mParams;
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
			
		} // private namespace
		
		//
		void ValidatePageStoreStringMap(const HermitPtr& h_,
										const stringmap::StringMapPtr& inStringMap,
										const stringmap::ValidateStringMapCompletionFunctionPtr& inCompletionFunction) {
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			
			auto completion = std::make_shared<CompletionProxy>(inStringMap, inCompletionFunction);
			auto task = std::make_shared<Task>(Params(h_, inStringMap, completion));
			if (!stringMap.QueueTask(task)) {
				NOTIFY_ERROR(h_, "stringMap.QueueTask failed");
				inCompletionFunction->Call(h_, stringmap::ValidateStringMapResult::kError);
			}
		}
		
	} // namespace pagestorestringmap
} // namespace hermit

