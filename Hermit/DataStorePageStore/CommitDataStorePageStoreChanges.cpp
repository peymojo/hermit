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

#include <set>
#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Encoding/CreateAlphaNumericID.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/Foundation/ParamBlock.h"
#include "Hermit/JSON/DataValueToJSON.h"
#include "Hermit/Value/Value.h"
#include "DataStorePageStore.h"
#include "CommitDataStorePageStoreChanges.h"

namespace hermit {
namespace datastorepagestore {

namespace {

	//
	typedef std::map<std::string, value::ValuePtr> ValueMap;
	typedef std::vector<value::ValuePtr> ValueVector;

	//
	DEFINE_PARAMBLOCK_3(CommitDataStorePageStoreChangesParams,
						HermitPtr, h_,
						pagestore::PageStorePtr, pageStore,
						pagestore::CommitPageStoreChangesCompletionFunctionPtr, completion);

	// proxy to avoid cancel during delete
	class DeleteItemH_Proxy : public Hermit {
	public:
		//
		DeleteItemH_Proxy(const HermitPtr& h_)
			:
			mH_(h_) {
		}
		
		//
		bool ShouldAbort() override {
			return false;
		}
		
		//
		void Notify(const char* name, const void* param) override {
			NOTIFY(mH_, name, param);
		}
		
		//
		HermitPtr mH_;
	};

	//
	class WriteIndexCompletion : public datastore::WriteDataStoreDataCompletionFunction
	{
	public:
		//
		WriteIndexCompletion(const CommitDataStorePageStoreChangesParams& inParams,
							 const DataStorePageStore::PageMap& inUpdatedPageTable,
							 const DataStorePageStore::PageMap& inObsoletePages) :
		mParams(inParams),
		mUpdatedPageTable(inUpdatedPageTable),
		mObsoletePages(inObsoletePages) {
		}
		
		//
		//
		virtual void Call(const HermitPtr& h_, const datastore::WriteDataStoreDataStatus& inStatus) override {

			if (inStatus == datastore::kWriteDataStoreDataStatus_Canceled) {
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
				return;
			}
			if (inStatus != datastore::kWriteDataStoreDataStatus_Success) {
				NOTIFY_ERROR(h_, "WriteIndexCompletion: WriteDataStoreData failed for index.");
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
				return;
			}
			
			DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mParams.pageStore);
			pageStore.mPageTable.swap(mUpdatedPageTable);
			pageStore.mDirtyPages.clear();
			
			//	remove obsolete pages. errors here are logged but otherwise ignored.
			if (!mObsoletePages.empty()) {
			
			
			
			
				{
					std::set<std::string> pages;
					{
						auto end = pageStore.mPageTable.end();
						for (auto it = pageStore.mPageTable.begin(); it != end; ++it) {
							if (!pages.insert(it->second).second) {
								NOTIFY_ERROR(h_, "WriteIndexCompletion: !pages.insert(it->second).second?");
								mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
								return;
							}
						}
					}
					{
						auto end = mObsoletePages.end();
						for (auto it = mObsoletePages.begin(); it != end; ++it) {
							if (pages.find(it->second) != pages.end()) {
								NOTIFY_ERROR(h_,
											 "pages.find(it->second) != pages.end() for key:",
											 it->first,
											 "value:",
											 it->second);
								mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
								return;
							}
						}
					}
				}
				
			
			
			
			
			
				datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagesPath;
				pageStore.mPath->AppendPathComponent(mParams.h_, "pages", pagesPath);
				if (!pagesPath.mSuccess) {
					NOTIFY_ERROR(h_, "WriteIndexCompletion: AppendToDataPath failed for pages path.");
				}
				else {
					auto end = mObsoletePages.end();
					for (auto it = mObsoletePages.begin(); it != end; ++it) {
						std::string pageName(it->second);
						datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagePath;
						if (pageName[0] == '#') {
							// support for older page store
							pageStore.mPath->AppendPathComponent(mParams.h_, pageName.c_str() + 1, pagePath);
						}
						else {
							pagesPath.mPath->AppendPathComponent(mParams.h_, pageName, pagePath);
						}
						if (!pagePath.mSuccess) {
							NOTIFY_ERROR(h_,
										 "WriteIndexCompletion: AppendToDataPath failed for pageName:",
										 pageName);
						}
						else {
							// use a proxy to avoid cancel during a delete, since these are obsolete
							// s3 objects and there's a cost associated with keeping them around
							auto proxy = std::make_shared<DeleteItemH_Proxy>(mParams.h_);
							if (!pageStore.mDataStore->DeleteItem(proxy, pagePath.mPath)) {
								NOTIFY_ERROR(h_,
											 "WriteIndexCompletion: DeleteDataStoreItem failed for pageName:",
											 pageName);
							}
						}
					}
				}
			}
			
			mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Success);
		}

		//
		CommitDataStorePageStoreChangesParams mParams;
		DataStorePageStore::PageMap mUpdatedPageTable;
		DataStorePageStore::PageMap mObsoletePages;
	};

	//
	void WritePageStoreIndex(const CommitDataStorePageStoreChangesParams& inParams,
							 const DataStorePageStore::PageMap& inUpdatedPageTable) {
		if (CHECK_FOR_ABORT(inParams.h_)) {
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
			return;
		}

		DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inParams.pageStore);

		DataStorePageStore::PageMap pageTable(inUpdatedPageTable);
		DataStorePageStore::PageMap obsoletePages;
		auto oldEnd = pageStore.mPageTable.end();
		for (auto oldIt = pageStore.mPageTable.begin(); oldIt != oldEnd; ++oldIt) {
			if (!pageTable.insert(*oldIt).second) {
				obsoletePages.insert(*oldIt);
			}
		}
		
		ValueVector pageVector;
		auto newEnd = pageTable.end();
		for (auto newIt = pageTable.begin(); newIt != newEnd; ++newIt) {
			value::ObjectValuePtr page(new value::ObjectValueClassT<ValueMap>());
			page->SetItem(inParams.h_, "key", value::StringValue::New(newIt->first));
			page->SetItem(inParams.h_, "name", value::StringValue::New(newIt->second));
			pageVector.push_back(page);
		}
		
		value::ArrayValuePtr pageArray(new value::ArrayValueClassT<ValueVector>(pageVector));
		value::ObjectValuePtr rootObject(new value::ObjectValueClassT<ValueMap>());
		rootObject->SetItem(inParams.h_, "pages", pageArray);

		std::string jsonString;
		json::DataValueToJSON(inParams.h_, rootObject, jsonString);
		if (jsonString.empty()) {
			NOTIFY_ERROR(inParams.h_, "CommitDataStorePageStoreChanges: DataValueToJSON failed.");
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
			return;
		}
		
		datastore::DataPathCallbackClassT<datastore::DataPathPtr> indexPath;
		pageStore.mPath->AppendPathComponent(inParams.h_, "index.json", indexPath);
		if (!indexPath.mSuccess) {
			NOTIFY_ERROR(inParams.h_, "CommitDataStorePageStoreChanges: AppendToDataPath failed for index.json.");
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
			return;
		}
		
		auto dataBuffer = std::make_shared<SharedBuffer>(jsonString.data(), jsonString.size());
		datastore::WriteDataStoreDataCompletionFunctionPtr completion(new WriteIndexCompletion(inParams,
																							   pageTable,
																							   obsoletePages));
		pageStore.mDataStore->WriteData(inParams.h_,
										indexPath.mPath,
										dataBuffer,
										datastore::kDataStoreEncryptionSetting_Default,
										completion);
	}

	//
	class CommitPageStoreAggregator {
	public:
		//
		CommitPageStoreAggregator(const CommitDataStorePageStoreChangesParams& inParams) :
		mParams(inParams),
		mTaskCount(0),
		mAllTasksAdded(false),
		mAllTasksReportedComplete(false),
		mPrimaryTaskFailed(false),
		mWriteStatus(datastore::kWriteDataStoreDataStatus_Unknown) {
		}
		
		//
		void TaskComplete(const datastore::WriteDataStoreDataStatus& inStatus,
						  const std::string& inKey,
						  const std::string& inName) {
			{
				ThreadLockScope lock(mLock);
				--mTaskCount;
				
				if ((mWriteStatus == datastore::kWriteDataStoreDataStatus_Unknown) ||
					(mWriteStatus == datastore::kWriteDataStoreDataStatus_Success)) {
					mWriteStatus = inStatus;
				}
				if (inStatus == datastore::kWriteDataStoreDataStatus_Success) {
					DataStorePageStore::PageMap::value_type value(inKey, inName);
					mUpdatedPageTable.insert(value);
				}
			}
			if (AreAllTasksComplete()) {
				AllTasksCompleted();
			}
		}
		
		//
		void TaskAdded() {
			ThreadLockScope lock(mLock);
			++mTaskCount;
		}
		
		//
		void AllTasksAdded() {
			{
				ThreadLockScope lock(mLock);
				mAllTasksAdded = true;
			}
			if (AreAllTasksComplete()) {
				AllTasksCompleted();
			}
		}
		
		//
		void PrimaryTaskFailed() {
			ThreadLockScope lock(mLock);
			mPrimaryTaskFailed = true;
		}
		
		//
		bool AreAllTasksComplete() {
			ThreadLockScope lock(mLock);
			if (!mAllTasksAdded || mAllTasksReportedComplete) {
				return false;
			}
			if (mTaskCount > 0) {
				return false;
			}
			
			mAllTasksReportedComplete = true;
			return true;
		}
		
		//
		void AllTasksCompleted() {
			ThreadLockScope lock(mLock);
			
			if (mWriteStatus == datastore::kWriteDataStoreDataStatus_Canceled) {
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
				return;
			}
			if (mPrimaryTaskFailed) {
				NOTIFY_ERROR(mParams.h_, "CommitDataStorePageStoreChanges: mPrimaryTaskFailed.");
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
				return;
			}
			if (mWriteStatus != datastore::kWriteDataStoreDataStatus_Success) {
				NOTIFY_ERROR(mParams.h_, "CommitDataStorePageStoreChanges: PageWrite failed.");
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
				return;
			}
			
			WritePageStoreIndex(mParams, mUpdatedPageTable);
		}
		
		//
		CommitDataStorePageStoreChangesParams mParams;
		ThreadLock mLock;
		int32_t mTaskCount;
		bool mAllTasksAdded;
		bool mAllTasksReportedComplete;
		bool mPrimaryTaskFailed;
		datastore::WriteDataStoreDataStatus mWriteStatus;
		DataStorePageStore::PageMap mUpdatedPageTable;
	};
	typedef std::shared_ptr<CommitPageStoreAggregator> CommitPageStoreAggregatorPtr;
	
	//
	class WriteCompletion : public datastore::WriteDataStoreDataCompletionFunction {
	public:
		//
		WriteCompletion(const std::string& inKey,
						const std::string& inName,
						const CommitPageStoreAggregatorPtr& inAggregator) :
		mKey(inKey),
		mName(inName),
		mAggregator(inAggregator) {
		}
		
		//
		virtual void Call(const HermitPtr& h_, const datastore::WriteDataStoreDataStatus& inStatus) override {
			mAggregator->TaskComplete(inStatus, mKey, mName);
		}

		//
		std::string mKey;
		std::string mName;
		CommitPageStoreAggregatorPtr mAggregator;
	};
	
	//
	void CommitChanges(const CommitDataStorePageStoreChangesParams& inParams) {
		if (CHECK_FOR_ABORT(inParams.h_)) {
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
			return;
		}
		
		DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inParams.pageStore);
		
		datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagesPath;
		pageStore.mPath->AppendPathComponent(inParams.h_, "pages", pagesPath);
		if (!pagesPath.mSuccess) {
			NOTIFY_ERROR(inParams.h_, "CommitChanges: AppendToDataPath failed for pages path.");
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
			return;
		}
		
		auto result = pageStore.mDataStore->CreateLocationIfNeeded(inParams.h_, pagesPath.mPath);
		if (result != datastore::kCreateDataStoreLocationIfNeededStatus_Success) {
			NOTIFY_ERROR(inParams.h_, "CommitChanges: CreateDataStoreLocationIfNeeded failed for pages path.");
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
			return;
		}

		CommitPageStoreAggregatorPtr aggregator(new CommitPageStoreAggregator(inParams));
		auto end = pageStore.mDirtyPages.end();
		for (auto it = pageStore.mDirtyPages.begin(); it != end; ++it) {
			std::string pageId;
			encoding::CreateAlphaNumericID(inParams.h_, 20, pageId);
			pageId += ".page";
		
			datastore::DataPathCallbackClassT<datastore::DataPathPtr> pagePath;
			pagesPath.mPath->AppendPathComponent(inParams.h_, pageId, pagePath);
			if (!pagePath.mSuccess) {
				NOTIFY_ERROR(inParams.h_,
							 "CommitChanges: AppendToDataPath failed for pageFileName:",
							 pageId);
				aggregator->PrimaryTaskFailed();
				break;
			}
		
			auto completion = std::make_shared<WriteCompletion>(it->first, pageId, aggregator);
			aggregator->TaskAdded();
			pageStore.mDataStore->WriteData(inParams.h_,
											pagePath.mPath,
											it->second,
											datastore::kDataStoreEncryptionSetting_Default,
											completion);
		}
		aggregator->AllTasksAdded();
	}

	//
	class TableLoaded : public ReadPageTableCompletionFunction {
	public:
		//
		TableLoaded(const CommitDataStorePageStoreChangesParams& inParams) : mParams(inParams) {
		}
		
		//
		virtual void Call(const ReadPageTableResult& inResult) override {
			if (inResult == kReadPageTableResult_Canceled) {
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
				return;
			}
			if (inResult != kReadPageTableResult_Success) {
				NOTIFY_ERROR(mParams.h_, "CommitDataStorePageStoreChanges: ReadPageTable failed");
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
				return;
			}
			CommitChanges(mParams);
		}
		
		//
		CommitDataStorePageStoreChangesParams mParams;
	};

	//
	void PerformWork(const CommitDataStorePageStoreChangesParams& inParams) {
		if (CHECK_FOR_ABORT(inParams.h_)) {
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
			return;
		}
		
		DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inParams.pageStore);
			
		if (pageStore.mDirtyPages.empty()) {
			inParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Success);
			return;
		}

		if (!pageStore.mPageTableLoaded) {
			auto completion = std::make_shared<TableLoaded>(inParams);
			pageStore.ReadPageTable(inParams.h_, completion);
			return;
		}

		CommitChanges(inParams);
	}
	
	//
	class UnlockingCompletion : public pagestore::CommitPageStoreChangesCompletionFunction {
	public:
		//
		UnlockingCompletion(const CommitDataStorePageStoreChangesParams& inParams) :
		mParams(inParams) {
		}
		
		//
		void Call(const pagestore::CommitPageStoreChangesStatus& inStatus) {
			mParams.pageStore->Unlock(mParams.h_);
			mParams.completion->Call(inStatus);
		}

		//
		CommitDataStorePageStoreChangesParams mParams;
	};
	
	//
	class LockCallback : public pagestore::LockPageStoreCompletionFunction {
	public:
		//
		LockCallback(const CommitDataStorePageStoreChangesParams& inParams) :
		mParams(inParams) {
		}
		
		//
		void Call(const pagestore::LockPageStoreStatus& inStatus) {
			if (inStatus == pagestore::kLockPageStoreStatus_Canceled) {
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Canceled);
				return;
			}
			if (inStatus != pagestore::kLockPageStoreStatus_Success) {
				NOTIFY_ERROR(mParams.h_, "CommitDataStorePageStoreChanges: LockPageStore failed.");
				mParams.completion->Call(pagestore::kCommitPageStoreChangesStatus_Error);
				return;
			}
			
			pagestore::CommitPageStoreChangesCompletionFunctionPtr unlockingCompletion(new UnlockingCompletion(mParams));
			mParams.completion = unlockingCompletion;
			PerformWork(mParams);
		}
		
		//
		CommitDataStorePageStoreChangesParams mParams;
	};
	
} // private namespace

//
//
void CommitDataStorePageStoreChanges(const HermitPtr& h_,
									 const pagestore::PageStorePtr& inPageStore,
									 const pagestore::CommitPageStoreChangesCompletionFunctionPtr& inCompletionFunction) {
	auto params = CommitDataStorePageStoreChangesParams(h_, inPageStore, inCompletionFunction);
	auto completion = std::make_shared<LockCallback>(params);
	inPageStore->Lock(h_, completion);
}

} // namespace datastorepagestore
} // namespace hermit
