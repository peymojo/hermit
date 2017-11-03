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

#include <string>
#include "Hermit/Foundation/AsyncTaskQueue.h"
#include "Hermit/Foundation/Notification.h"
#include "DataStorePageStore.h"
#include "EnumerateDataStorePageStorePages.h"

namespace hermit {
	namespace datastorepagestore {
		
		namespace {
			
			//
			void EnumeratePages(const HermitPtr& h_,
								const pagestore::PageStorePtr& inPageStore,
								const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
								const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) {
				if (CHECK_FOR_ABORT(h_)) {
					inCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Canceled);
					return;
				}
				
				DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
				auto end = pageStore.mPageTable.end();
				for (auto it = pageStore.mPageTable.begin(); it != end; ++it) {
					if (!inEnumerationFunction->Call(it->first)) {
						inCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Canceled);
						return;
					}
				}
				inCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Success);
			}
			
			//
			class TableLoaded : public ReadPageTableCompletionFunction {
			public:
				//
				TableLoaded(const pagestore::PageStorePtr& inPageStore,
							const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
							const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) :
				mPageStore(inPageStore),
				mEnumerationFunction(inEnumerationFunction),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const ReadPageTableResult& inResult) override {
					if (inResult == ReadPageTableResult::kCanceled) {
						mCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Canceled);
						return;
					}
					if (inResult != ReadPageTableResult::kSuccess) {
						NOTIFY_ERROR(h_, "EnumerateDataStorePageStorePages: ReadPageTable failed");
						mCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Error);
						return;
					}
					EnumeratePages(h_, mPageStore, mEnumerationFunction, mCompletionFunction);
				}
				
				//
				pagestore::PageStorePtr mPageStore;
				pagestore::EnumeratePageStorePagesEnumerationFunctionPtr mEnumerationFunction;
				pagestore::EnumeratePageStorePagesCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			void PerformWork(const HermitPtr& h_,
							 const pagestore::PageStorePtr& inPageStore,
							 const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
							 const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) {
				if (CHECK_FOR_ABORT(h_)) {
					inCompletionFunction->Call(h_, pagestore::kEnumeratePageStorePagesResult_Canceled);
					return;
				}
				
				DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
				
				if (!pageStore.mPageTableLoaded) {
					DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
					auto completion = std::make_shared<TableLoaded>(inPageStore,
																	inEnumerationFunction,
																	inCompletionFunction);
					pageStore.ReadPageTable(h_, completion);
					return;
				}
				
				EnumeratePages(h_, inPageStore, inEnumerationFunction, inCompletionFunction);
			}
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const HermitPtr& h_,
					 const pagestore::PageStorePtr& inPageStore,
					 const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
					 const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mPageStore(inPageStore),
				mEnumerationFunction(inEnumerationFunction),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				void PerformTask(const int32_t& inTaskID) {
					PerformWork(mH_, mPageStore, mEnumerationFunction, mCompletionFunction);
				}
				
				//
				HermitPtr mH_;
				pagestore::PageStorePtr mPageStore;
				pagestore::EnumeratePageStorePagesEnumerationFunctionPtr mEnumerationFunction;
				pagestore::EnumeratePageStorePagesCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class CompletionProxy : public pagestore::EnumeratePageStorePagesCompletionFunction {
			public:
				//
				CompletionProxy(const pagestore::PageStorePtr& inPageStore,
								const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) :
				mPageStore(inPageStore),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const pagestore::EnumeratePageStorePagesResult& inResult) override {
					mCompletionFunction->Call(h_, inResult);
					
					DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
					pageStore.TaskComplete();
				}
				
				//
				//
				pagestore::PageStorePtr mPageStore;
				pagestore::EnumeratePageStorePagesCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		void EnumerateDataStorePageStorePages(const HermitPtr& h_,
											  const pagestore::PageStorePtr& inPageStore,
											  const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
											  const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) {
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			
			pagestore::EnumeratePageStorePagesCompletionFunctionPtr proxy(new CompletionProxy(inPageStore, inCompletionFunction));
			auto task = std::make_shared<Task>(h_, inPageStore, inEnumerationFunction, proxy);
			if (!pageStore.QueueTask(task)) {
				NOTIFY_ERROR(h_, "pageStore.QueueTask failed");
				inCompletionFunction->Call(h_, pagestore::EnumeratePageStorePagesResult::kEnumeratePageStorePagesResult_Error);
			}
		}
		
	} // namespace datastorepagestore
} // namespace hermit
