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

#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/ParamBlock.h"
#include "DataStorePageStore.h"
#include "ValidateDataStorePageStore.h"

namespace hermit {
namespace datastorepagestore {

namespace {

	//
	DEFINE_PARAMBLOCK_3(Params,
						HermitPtr, h_,
						pagestore::PageStorePtr, pageStore,
						pagestore::ValidatePageStoreCompletionFunctionPtr, completion);

	//
	void ValidatePages(const Params& inParams) {
		if (CHECK_FOR_ABORT(inParams.h_)) {
			inParams.completion->Call(pagestore::kValidatePageStoreStatus_Canceled);
			return;
		}

		DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inParams.pageStore);
		datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagesPath;
		pageStore.mPath->AppendPathComponent(inParams.h_, "pages", pagesPath);
		if (!pagesPath.mSuccess) {
			NOTIFY_ERROR(inParams.h_, "ValidatePages: AppendToDataPath failed for pages path.");
			inParams.completion->Call(pagestore::kValidatePageStoreStatus_Error);
			return;
		}

		auto end = pageStore.mPageTable.end();
		for (auto it = pageStore.mPageTable.begin(); it != end; ++it) {
			std::string pageFileName = it->second;
			
			datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagePath;
			if (pageFileName[0] == '#') {
				//	transitional page store without "pages" subdir for older pages
				pageStore.mPath->AppendPathComponent(inParams.h_, pageFileName.c_str() + 1, pagePath);
			}
			else {
				pagesPath.mPath->AppendPathComponent(inParams.h_, pageFileName, pagePath);
			}
			
			if (!pagePath.mSuccess) {
				NOTIFY_ERROR(inParams.h_,
							 "ValidatePages: AppendToDataPath failed, continuing, pageFileName:",
							 pageFileName);
			}
			else {
				datastore::ItemExistsInDataStoreCallbackClass exists;
				pageStore.mDataStore->ItemExists(inParams.h_, pagePath.mPath, exists);
				if (exists.mStatus == datastore::ItemExistsInDataStoreStatus::kCanceled) {
					inParams.completion->Call(pagestore::kValidatePageStoreStatus_Canceled);
					return;
				}
				if (exists.mStatus != datastore::ItemExistsInDataStoreStatus::kSuccess) {
					NOTIFY_ERROR(inParams.h_,
								 "ValidatePages: ItemExistsInDataStore failed, continuing, pageFileName:",
								 pageFileName);
				}
				else if (!exists.mExists) {
					NOTIFY_ERROR(inParams.h_,
								 "ValidatePages: Page missing, continuing, pageFileName:",
								 pageFileName);
				}
			}
		}
		inParams.completion->Call(pagestore::kValidatePageStoreStatus_Success);
	}

	//
	class TableLoaded : public ReadPageTableCompletionFunction {
	public:
		//
		TableLoaded(const Params& inParams) : mParams(inParams) {
		}
		
		//
		virtual void Call(const ReadPageTableResult& inResult) override {
			if (inResult == kReadPageTableResult_Canceled) {
				mParams.completion->Call(pagestore::kValidatePageStoreStatus_Canceled);
				return;
			}
			if (inResult != kReadPageTableResult_Success) {
				NOTIFY_ERROR(mParams.h_, "ValidateDataStorePageStore: ReadPageTable failed");
				mParams.completion->Call(pagestore::kValidatePageStoreStatus_Error);
				return;
			}
			ValidatePages(mParams);
		}
		
		//
		Params mParams;
	};

	//
	class Task : public AsyncTask {
	public:
		//
		Task(const Params& inParams) : mParams(inParams) {
		}

		//
		void PerformTask(const int32_t& inTaskID) {
			if (CHECK_FOR_ABORT(mParams.h_)) {
				mParams.completion->Call(pagestore::kValidatePageStoreStatus_Canceled);
				return;
			}
		
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mParams.pageStore);
			if (!pageStore.mPageTableLoaded) {
				auto completion = std::make_shared<TableLoaded>(mParams);
				pageStore.ReadPageTable(mParams.h_, completion);
				return;
			}
			ValidatePages(mParams);
		}
		
		//
		Params mParams;
	};

	//
	class CompletionProxy : public pagestore::ValidatePageStoreCompletionFunction {
	public:
		//
		CompletionProxy(const pagestore::PageStorePtr& inPageStore,
						const pagestore::ValidatePageStoreCompletionFunctionPtr& inCompletionFunction) :
		mPageStore(inPageStore),
		mCompletionFunction(inCompletionFunction) {
		}

		//
		virtual void Call(const pagestore::ValidatePageStoreStatus& inStatus) override {
			mCompletionFunction->Call(inStatus);
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
			pageStore.TaskComplete();
		}
		
		//
		pagestore::PageStorePtr mPageStore;
		pagestore::ValidatePageStoreCompletionFunctionPtr mCompletionFunction;
	};

} // private namespace

//
void ValidateDataStorePageStore(const HermitPtr& h_,
								const pagestore::PageStorePtr& inPageStore,
								const pagestore::ValidatePageStoreCompletionFunctionPtr& inCompletionFunction) {
	DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
	auto proxy = std::make_shared<CompletionProxy>(inPageStore, inCompletionFunction);
	auto task = std::make_shared<Task>(Params(h_, inPageStore, proxy));
	if (!pageStore.QueueTask(task)) {
		NOTIFY_ERROR(h_, "pageStore.QueueTask failed");
		inCompletionFunction->Call(pagestore::ValidatePageStoreStatus::kValidatePageStoreStatus_Error);
	}
}

} // namespace datastorepagestore
} // namespace hermit
