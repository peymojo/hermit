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
#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/QueueAsyncTask.h"
#include "DataStorePageStore.h"
#include "ReadDataStorePageStorePage.h"

namespace hermit {
	namespace datastorepagestore {
		
		//
		class CompletionBlock : public datastore::LoadDataStoreDataCompletionBlock {
		public:
			//
			CompletionBlock(const std::string& inPageFileName,
							const datastore::LoadDataStoreDataDataPtr& inData,
							const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletion) :
			mPageFileName(inPageFileName),
			mData(inData),
			mCompletion(inCompletion) {
			}
				
			//
			virtual void Call(const HermitPtr& h_, const datastore::LoadDataStoreDataStatus& status) override {
				if (status == datastore::LoadDataStoreDataStatus::kCanceled) {
					mCompletion->Call(h_, pagestore::ReadPageStorePageResult::kCanceled, DataBuffer());
					return;
				}
				if (status == datastore::LoadDataStoreDataStatus::kItemNotFound) {
					NOTIFY_ERROR(h_, "ReadPageData: Page missing? name:", mPageFileName);
					mCompletion->Call(h_, pagestore::ReadPageStorePageResult::kPageNotFound, DataBuffer());
					return;
				}
				if (status != datastore::LoadDataStoreDataStatus::kSuccess) {
					NOTIFY_ERROR(h_, "ReadPageData: LoadDataStoreData failed, pageFileName:", mPageFileName);
					mCompletion->Call(h_, pagestore::ReadPageStorePageResult::kError, DataBuffer());
					return;
				}
				auto dataBuffer = DataBuffer(mData->mData.data(), mData->mData.size());
				mCompletion->Call(h_, pagestore::ReadPageStorePageResult::kSuccess, dataBuffer);
			}
				
			//
			std::string mPageFileName;
			datastore::LoadDataStoreDataDataPtr mData;
			pagestore::ReadPageStorePageCompletionFunctionPtr mCompletion;
		};
			
		//
		void ReadPageData(const HermitPtr& h_,
						  const pagestore::PageStorePtr& inPageStore,
						  const std::string& inPageName,
						  const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) {
			if (CHECK_FOR_ABORT(h_)) {
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kCanceled, DataBuffer());
				return;
			}
				
			datastore::DataStorePtr dataStore;
			datastore::DataPathPtr path;
			std::string pageFileName;
				
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
				
			auto dataIt = pageStore.mDirtyPages.find(inPageName);
			if (dataIt != pageStore.mDirtyPages.end()) {
				inCompletionFunction->Call(h_,
										   pagestore::ReadPageStorePageResult::kSuccess,
										   DataBuffer(dataIt->second->Data(), dataIt->second->Size()));
						
				return;
			}
					
			auto it = pageStore.mPageTable.find(inPageName);
			if (it == pageStore.mPageTable.end()) {
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kPageNotFound, DataBuffer());
				return;
			}
					
			dataStore = pageStore.mDataStore;
			path = pageStore.mPath;
			pageFileName = it->second;
				
			datastore::DataPathPtr pagesPath;
			if (!path->AppendPathComponent(h_, "pages", pagesPath)) {
				NOTIFY_ERROR(h_, "ReadPageData: AppendToDataPath failed for pages path.");
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kError, DataBuffer());
				return;
			}
				
			datastore::DataPathPtr pagePath;
			bool success = false;
			if (pageFileName[0] == '#') {
				// older page stores had page files at the root rather than under the "pages" folder
				success = path->AppendPathComponent(h_, pageFileName.c_str() + 1, pagePath);
			}
			else {
				success = pagesPath->AppendPathComponent(h_, pageFileName, pagePath);
			}
			if (!success) {
				NOTIFY_ERROR(h_, "ReadPageData: AppendToDataPath failed for pageFileName:", pageFileName);
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kError, DataBuffer());
				return;
			}
				
			auto dataBlock = std::make_shared<datastore::LoadDataStoreDataData>();
			auto completion = std::make_shared<CompletionBlock>(pageFileName, dataBlock, inCompletionFunction);
			auto encryptionSetting = datastore::EncryptionSetting::kDefault;
			dataStore->LoadData(h_, pagePath, encryptionSetting, dataBlock, completion);
		}
			
		//
		class TableLoaded : public ReadPageTableCompletionFunction {
		public:
			//
			TableLoaded(const pagestore::PageStorePtr& inPageStore,
						const std::string& inPageName,
						const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) :
			mPageStore(inPageStore),
			mPageName(inPageName),
			mCompletionFunction(inCompletionFunction) {
			}
				
			//
			virtual void Call(const HermitPtr& h_, const ReadPageTableResult& inResult) override {
				if (inResult == ReadPageTableResult::kCanceled) {
					mCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kCanceled, DataBuffer());
					return;
				}
				if (inResult != ReadPageTableResult::kSuccess) {
					NOTIFY_ERROR(h_, "ReadDataStorePageStorePage: ReadPageTable failed");
					mCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kError, DataBuffer());
					return;
				}
				ReadPageData(h_, mPageStore, mPageName, mCompletionFunction);
			}
				
			//
			pagestore::PageStorePtr mPageStore;
			std::string mPageName;
			pagestore::ReadPageStorePageCompletionFunctionPtr mCompletionFunction;
		};
			
		//
		void PerformWork(const HermitPtr& h_,
						 const pagestore::PageStorePtr& inPageStore,
						 const std::string& inPageName,
						 const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) {
			if (CHECK_FOR_ABORT(h_)) {
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kCanceled, DataBuffer());
				return;
			}
				
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			if (!pageStore.mPageTableLoaded) {
				auto completion = std::make_shared<TableLoaded>(inPageStore, inPageName, inCompletionFunction);
				pageStore.ReadPageTable(h_, completion);
				return;
			}
			ReadPageData(h_, inPageStore, inPageName, inCompletionFunction);
		}
			
		//
		class Task : public AsyncTask {
		public:
			//
			Task(const HermitPtr& h_,
				 const pagestore::PageStorePtr& inPageStore,
				 const std::string& inPageName,
				 const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) :
			mH_(h_),
			mPageStore(inPageStore),
			mPageName(inPageName),
			mCompletionFunction(inCompletionFunction) {
			}
				
			//
			void PerformTask(const int32_t& inTaskID) {
				PerformWork(mH_, mPageStore, mPageName, mCompletionFunction);
			}
				
			//
			HermitPtr mH_;
			pagestore::PageStorePtr mPageStore;
			std::string mPageName;
			pagestore::ReadPageStorePageCompletionFunctionPtr mCompletionFunction;
		};
			
		//
		class CompletionProxy : public pagestore::ReadPageStorePageCompletionFunction {
		public:
			//
			CompletionProxy(const pagestore::PageStorePtr& inPageStore,
							const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) :
			mPageStore(inPageStore),
			mCompletionFunction(inCompletionFunction) {
			}
				
			//
			virtual void Call(const HermitPtr& h_,
							  const pagestore::ReadPageStorePageResult& inResult,
							  const DataBuffer& inPageData) override {
				mCompletionFunction->Call(h_, inResult, inPageData);
				DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
				pageStore.TaskComplete();
			}
				
			//
			pagestore::PageStorePtr mPageStore;
			pagestore::ReadPageStorePageCompletionFunctionPtr mCompletionFunction;
		};
		
		//
		void ReadDataStorePageStorePage(const HermitPtr& h_,
										const pagestore::PageStorePtr& inPageStore,
										const std::string& inPageName,
										const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) {
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
			auto proxy = std::make_shared<CompletionProxy>(inPageStore, inCompletionFunction);
			auto task = std::make_shared<Task>(h_, inPageStore, inPageName, proxy);
			if (!pageStore.QueueTask(task)) {
				NOTIFY_ERROR(h_, "pageStore.QueueTask failed");
				inCompletionFunction->Call(h_, pagestore::ReadPageStorePageResult::kError, DataBuffer());
			}
		}
		
	} // namespace datastorepagestore
} // namespace hermit
