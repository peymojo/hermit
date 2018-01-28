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
#include "WriteDataStorePageStorePage.h"

namespace hermit {
	namespace datastorepagestore {
		
		namespace {
			
			//
			void PerformWork(const HermitPtr& h_,
							 const pagestore::PageStorePtr& inPageStore,
							 const std::string& inPageName,
							 const SharedBufferPtr& inPageData,
							 const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) {
				if (CHECK_FOR_ABORT(h_)) {
					inCompletionFunction->Call(pagestore::kWritePageStorePageResult_Canceled);
					return;
				}
				
				DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
				auto it = pageStore.mDirtyPages.find(inPageName);
				if (it != pageStore.mDirtyPages.end()) {
					it->second = inPageData;
				}
				else {
					pageStore.mDirtyPages.insert(DataStorePageStore::PageDataMap::value_type(inPageName, inPageData));
				}
				inCompletionFunction->Call(pagestore::kWritePageStorePageResult_Success);
			}
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const pagestore::PageStorePtr& inPageStore,
					 const std::string& inPageName,
					 const DataBuffer& inPageData,
					 const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) :
				mPageStore(inPageStore),
				mPageName(inPageName),
				mPageData(std::make_shared<SharedBuffer>(inPageData.first, inPageData.second)),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_, mPageStore, mPageName, mPageData, mCompletionFunction);
				}
				
				//
				pagestore::PageStorePtr mPageStore;
				std::string mPageName;
				SharedBufferPtr mPageData;
				pagestore::WritePageStorePageCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class CompletionProxy : public pagestore::WritePageStorePageCompletionFunction {
			public:
				//
				CompletionProxy(const pagestore::PageStorePtr& inPageStore,
								const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) :
				mPageStore(inPageStore),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const pagestore::WritePageStorePageResult& inResult) override {
					mCompletionFunction->Call(inResult);
					DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
					pageStore.TaskComplete();
				}
				
				//
				pagestore::PageStorePtr mPageStore;
				pagestore::WritePageStorePageCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		void WriteDataStorePageStorePage(const HermitPtr& h_,
										 const pagestore::PageStorePtr& inPageStore,
										 const std::string& inPageName,
										 const DataBuffer& inPageData,
										 const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) {
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			pagestore::WritePageStorePageCompletionFunctionPtr proxy(new CompletionProxy(inPageStore, inCompletionFunction));
			auto task = std::make_shared<Task>(inPageStore,  inPageName, inPageData, proxy);
			if (!pageStore.QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "pageStore.QueueTask failed");
				inCompletionFunction->Call(pagestore::WritePageStorePageResult::kWritePageStorePageResult_Error);
			}
		}
		
	} // namespace datastorepagestore
} // namespace hermit
