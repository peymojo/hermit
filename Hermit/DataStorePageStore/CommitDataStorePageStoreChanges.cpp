//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <set>
#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Encoding/CreateAlphaNumericID.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/JSON/DataValueToJSON.h"
#include "Hermit/Value/Value.h"
#include "DataStorePageStore.h"
#include "CommitDataStorePageStoreChanges.h"

namespace hermit {
    namespace datastorepagestore {
        namespace CommitDataStorePageStoreChanges_Impl {
            
            //
            typedef std::map<std::string, value::ValuePtr> ValueMap;
            typedef std::vector<value::ValuePtr> ValueVector;
            
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
            class PageDeleter;
            typedef std::shared_ptr<PageDeleter> PageDeleterPtr;
            
            //
            class PageDeleter : public std::enable_shared_from_this<PageDeleter> {
            public:
                //
                PageDeleter(const pagestore::PageStorePtr& pageStore,
                            const DataStorePageStore::PageMap& obsoletePages,
                            const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mObsoletePages(obsoletePages),
                mCompletion(completion) {
                }
                
                //
                void DeletePages(const HermitPtr& h_) {
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    if (!pageStore.mPath->AppendPathComponent(h_, "pages", mPagesPath)) {
                        NOTIFY_ERROR(h_, "AppendPathComponent failed for pages path.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kSuccess);
                        return;
                    }

                    mPageIterator = mObsoletePages.begin();
                    DeleteNextPage(h_);
                }
                
                //
                class DeleteCompletion : public datastore::DeleteDataStoreItemCompletion {
                public:
                    //
                    DeleteCompletion(const PageDeleterPtr& deleter, const std::string& pageName) :
                    mDeleter(deleter),
                    mPageName(pageName) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_, const datastore::DeleteDataStoreItemResult& result) override {
                        mDeleter->DeleteItemCompletion(h_, mPageName, result);
                    }
                    
                    //
                    PageDeleterPtr mDeleter;
                    std::string mPageName;
                };
                
                //
                void DeleteItemCompletion(const HermitPtr& h_,
                                          const std::string& pageName,
                                          const datastore::DeleteDataStoreItemResult& result) {
                    if (result != datastore::DeleteDataStoreItemResult::kSuccess) {
                        NOTIFY_ERROR(h_, "DeleteItem failed for pageName:", pageName);
                    }
                    DeleteNextPage(h_);
                }
                
                //
                void DeleteNextPage(const HermitPtr& h_) {
                    if (mPageIterator == mObsoletePages.end()) {
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kSuccess);
                        return;
                    }
                    std::string pageName(mPageIterator->second);
                    mPageIterator++;
                    
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    datastore::DataPathPtr pagePath;
                    bool success = false;
                    if (pageName[0] == '#') {
                        // support for older page store
                        success = pageStore.mPath->AppendPathComponent(h_, pageName.c_str() + 1, pagePath);
                    }
                    else {
                        success = mPagesPath->AppendPathComponent(h_, pageName, pagePath);
                    }
                    if (!success) {
                        NOTIFY_ERROR(h_, "AppendPathComponent failed for pageName:", pageName);
                        DeleteNextPage(h_);
                        return;
                    }

                    // use a proxy to avoid cancel during a delete, since these are obsolete
                    // s3 objects and there's a cost associated with keeping them around
                    auto proxy = std::make_shared<DeleteItemH_Proxy>(h_);
                    auto deleteCompletion = std::make_shared<DeleteCompletion>(shared_from_this(), pageName);
                    pageStore.mDataStore->DeleteItem(proxy, pagePath, deleteCompletion);
                }
                    
                //
                pagestore::PageStorePtr mPageStore;
                DataStorePageStore::PageMap mObsoletePages;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
                datastore::DataPathPtr mPagesPath;
                DataStorePageStore::PageMap::const_iterator mPageIterator;
            };
            
            //
            class WriteIndexCompletion : public datastore::WriteDataStoreDataCompletionFunction
            {
            public:
                //
                WriteIndexCompletion(const pagestore::PageStorePtr& pageStore,
                                     const DataStorePageStore::PageMap& inUpdatedPageTable,
                                     const DataStorePageStore::PageMap& inObsoletePages,
                                     const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mUpdatedPageTable(inUpdatedPageTable),
                mObsoletePages(inObsoletePages),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const datastore::WriteDataStoreDataResult& result) override {
                    if (result == datastore::WriteDataStoreDataResult::kCanceled) {
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                        return;
                    }
                    if (result != datastore::WriteDataStoreDataResult::kSuccess) {
                        NOTIFY_ERROR(h_, "WriteIndexCompletion: WriteDataStoreData failed for index.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    pageStore.mPageTable.swap(mUpdatedPageTable);
                    pageStore.mDirtyPages.clear();
                    
                    // remove obsolete pages. errors here are logged but otherwise ignored.
                    if (!mObsoletePages.empty()) {
                        
                        // make sure the obsolete pages are really obsolete, abort if not
                        {
                            std::set<std::string> pages;
                            {
                                auto end = pageStore.mPageTable.end();
                                for (auto it = pageStore.mPageTable.begin(); it != end; ++it) {
                                    if (!pages.insert(it->second).second) {
                                        NOTIFY_ERROR(h_, "WriteIndexCompletion: !pages.insert(it->second).second?");
                                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                                        return;
                                    }
                                }
                            }
                            {
                                auto end = mObsoletePages.end();
                                for (auto it = mObsoletePages.begin(); it != end; ++it) {
                                    if (pages.find(it->second) != pages.end()) {
                                        NOTIFY_ERROR(h_,
                                                     "pages.find(it->second) != pages.end() for key:", it->first,
                                                     "value:", it->second);
                                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                                        return;
                                    }
                                }
                            }
                        }
                        
                        auto deleter = std::make_shared<PageDeleter>(mPageStore, mObsoletePages, mCompletion);
                        deleter->DeletePages(h_);
                        return;
                    }
                    
                    mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kSuccess);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                DataStorePageStore::PageMap mUpdatedPageTable;
                DataStorePageStore::PageMap mObsoletePages;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
            };
            
            //
            void WritePageStoreIndex(const HermitPtr& h_,
                                     const pagestore::PageStorePtr& pageStore,
                                     const DataStorePageStore::PageMap& inUpdatedPageTable,
                                     const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) {
                if (CHECK_FOR_ABORT(h_)) {
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                    return;
                }
                
                DataStorePageStore& dataStorePageStore = static_cast<DataStorePageStore&>(*pageStore);
                
                DataStorePageStore::PageMap pageTable(inUpdatedPageTable);
                DataStorePageStore::PageMap obsoletePages;
                auto oldEnd = dataStorePageStore.mPageTable.end();
                for (auto oldIt = dataStorePageStore.mPageTable.begin(); oldIt != oldEnd; ++oldIt) {
                    if (!pageTable.insert(*oldIt).second) {
                        obsoletePages.insert(*oldIt);
                    }
                }
                
                ValueVector pageVector;
                auto newEnd = pageTable.end();
                for (auto newIt = pageTable.begin(); newIt != newEnd; ++newIt) {
                    value::ObjectValuePtr page(new value::ObjectValueClassT<ValueMap>());
                    page->SetItem(h_, "key", value::StringValue::New(newIt->first));
                    page->SetItem(h_, "name", value::StringValue::New(newIt->second));
                    pageVector.push_back(page);
                }
                
                value::ArrayValuePtr pageArray(new value::ArrayValueClassT<ValueVector>(pageVector));
                value::ObjectValuePtr rootObject(new value::ObjectValueClassT<ValueMap>());
                rootObject->SetItem(h_, "pages", pageArray);
                
                std::string jsonString;
                json::DataValueToJSON(h_, rootObject, jsonString);
                if (jsonString.empty()) {
                    NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: DataValueToJSON failed.");
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                    return;
                }
                
                datastore::DataPathPtr indexPath;
                if (!dataStorePageStore.mPath->AppendPathComponent(h_, "index.json", indexPath)) {
                    NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: AppendToDataPath failed for index.json.");
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                    return;
                }
                
                auto dataBuffer = std::make_shared<SharedBuffer>(jsonString.data(), jsonString.size());
                auto writeCompletion = std::make_shared<WriteIndexCompletion>(pageStore,
                                                                              pageTable,
                                                                              obsoletePages,
                                                                              completion);
                dataStorePageStore.mDataStore->WriteData(h_,
                                                         indexPath,
                                                         dataBuffer,
                                                         datastore::EncryptionSetting::kDefault,
                                                         writeCompletion);
            }
            
            //
            class CommitPageStoreAggregator {
            public:
                //
                CommitPageStoreAggregator(const pagestore::PageStorePtr& pageStore,
                                          const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion),
                mTaskCount(0),
                mAllTasksAdded(false),
                mAllTasksReportedComplete(false),
                mPrimaryTaskFailed(false),
                mWriteResult(datastore::WriteDataStoreDataResult::kUnknown) {
                }
                
                //
                void TaskComplete(const HermitPtr& h_,
                                  const datastore::WriteDataStoreDataResult& result,
                                  const std::string& inKey,
                                  const std::string& inName) {
                    {
                        ThreadLockScope lock(mLock);
                        --mTaskCount;
                        
                        if ((mWriteResult == datastore::WriteDataStoreDataResult::kUnknown) ||
                            (mWriteResult == datastore::WriteDataStoreDataResult::kSuccess)) {
                            mWriteResult = result;
                        }
                        if (result == datastore::WriteDataStoreDataResult::kSuccess) {
                            DataStorePageStore::PageMap::value_type value(inKey, inName);
                            mUpdatedPageTable.insert(value);
                        }
                    }
                    if (AreAllTasksComplete()) {
                        AllTasksCompleted(h_);
                    }
                }
                
                //
                void TaskAdded() {
                    ThreadLockScope lock(mLock);
                    ++mTaskCount;
                }
                
                //
                void AllTasksAdded(const HermitPtr& h_) {
                    {
                        ThreadLockScope lock(mLock);
                        mAllTasksAdded = true;
                    }
                    if (AreAllTasksComplete()) {
                        AllTasksCompleted(h_);
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
                void AllTasksCompleted(const HermitPtr& h_) {
                    ThreadLockScope lock(mLock);
                    
                    if (mWriteResult == datastore::WriteDataStoreDataResult::kCanceled) {
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                        return;
                    }
                    if (mPrimaryTaskFailed) {
                        NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: mPrimaryTaskFailed.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    if (mWriteResult != datastore::WriteDataStoreDataResult::kSuccess) {
                        NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: PageWrite failed.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    
                    WritePageStoreIndex(h_, mPageStore, mUpdatedPageTable, mCompletion);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
                ThreadLock mLock;
                int32_t mTaskCount;
                bool mAllTasksAdded;
                bool mAllTasksReportedComplete;
                bool mPrimaryTaskFailed;
                datastore::WriteDataStoreDataResult mWriteResult;
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
                virtual void Call(const HermitPtr& h_, const datastore::WriteDataStoreDataResult& result) override {
                    mAggregator->TaskComplete(h_, result, mKey, mName);
                }
                
                //
                std::string mKey;
                std::string mName;
                CommitPageStoreAggregatorPtr mAggregator;
            };
            
            //
            class CreateCompletion : public datastore::CreateDataStoreLocationIfNeededCompletion {
            public:
                //
                CreateCompletion(const pagestore::PageStorePtr& pageStore,
                                 const datastore::DataPathPtr& pagesPath,
                                 const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mPagesPath(pagesPath),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const datastore::CreateDataStoreLocationIfNeededResult& result) override {
                    if (result != datastore::CreateDataStoreLocationIfNeededResult::kSuccess) {
                        NOTIFY_ERROR(h_, "CommitChanges: CreateDataStoreLocationIfNeeded failed for pages path.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    
                    DataStorePageStore& dataStorePageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    CommitPageStoreAggregatorPtr aggregator(new CommitPageStoreAggregator(mPageStore, mCompletion));
                    auto end = dataStorePageStore.mDirtyPages.end();
                    for (auto it = dataStorePageStore.mDirtyPages.begin(); it != end; ++it) {
                        std::string pageId;
                        encoding::CreateAlphaNumericID(h_, 20, pageId);
                        pageId += ".page";
                        
                        datastore::DataPathPtr pagePath;
                        if (!mPagesPath->AppendPathComponent(h_, pageId, pagePath)) {
                            NOTIFY_ERROR(h_, "CommitChanges: AppendToDataPath failed for pageFileName:", pageId);
                            aggregator->PrimaryTaskFailed();
                            break;
                        }
                        
                        auto completion = std::make_shared<WriteCompletion>(it->first, pageId, aggregator);
                        aggregator->TaskAdded();
                        dataStorePageStore.mDataStore->WriteData(h_,
                                                                 pagePath,
                                                                 it->second,
                                                                 datastore::EncryptionSetting::kDefault,
                                                                 completion);
                    }
                    aggregator->AllTasksAdded(h_);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                datastore::DataPathPtr mPagesPath;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
            };
            
            //
            void CommitChanges(const HermitPtr& h_,
                               const pagestore::PageStorePtr& pageStore,
                               const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) {
                if (CHECK_FOR_ABORT(h_)) {
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                    return;
                }
                
                DataStorePageStore& dataStorePageStore = static_cast<DataStorePageStore&>(*pageStore);
                
                datastore::DataPathPtr pagesPath;
                if (!dataStorePageStore.mPath->AppendPathComponent(h_, "pages", pagesPath)) {
                    NOTIFY_ERROR(h_, "CommitChanges: AppendToDataPath failed for pages path.");
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                    return;
                }
                
                auto createCompletion = std::make_shared<CreateCompletion>(pageStore, pagesPath, completion);
                dataStorePageStore.mDataStore->CreateLocationIfNeeded(h_, pagesPath, createCompletion);
            }
            
            //
            class TableLoaded : public ReadPageTableCompletionFunction {
            public:
                //
                TableLoaded(const pagestore::PageStorePtr& pageStore,
                            const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const ReadPageTableResult& inResult) override {
                    if (inResult == ReadPageTableResult::kCanceled) {
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                        return;
                    }
                    if (inResult != ReadPageTableResult::kSuccess) {
                        NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: ReadPageTable failed");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    CommitChanges(h_, mPageStore, mCompletion);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
            };
            
            //
            void PerformWork(const HermitPtr& h_,
                             const pagestore::PageStorePtr& pageStore,
                             const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) {
                if (CHECK_FOR_ABORT(h_)) {
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                    return;
                }
                
                DataStorePageStore& dataStorePageStore = static_cast<DataStorePageStore&>(*pageStore);
                
                if (dataStorePageStore.mDirtyPages.empty()) {
                    completion->Call(h_, pagestore::CommitPageStoreChangesResult::kSuccess);
                    return;
                }
                
                if (!dataStorePageStore.mPageTableLoaded) {
                    auto readTableCompletion = std::make_shared<TableLoaded>(pageStore, completion);
                    dataStorePageStore.ReadPageTable(h_, readTableCompletion);
                    return;
                }
                
                CommitChanges(h_, pageStore, completion);
            }
            
            //
            class UnlockingCompletion : public pagestore::CommitPageStoreChangesCompletionFunction {
            public:
                //
                UnlockingCompletion(const pagestore::PageStorePtr& pageStore,
                                    const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const pagestore::CommitPageStoreChangesResult& inResult) override {
                    mPageStore->Unlock(h_);
                    mCompletion->Call(h_, inResult);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
            };
            
            //
            class LockCallback : public pagestore::LockPageStoreCompletionFunction {
            public:
                //
                LockCallback(const pagestore::PageStorePtr& pageStore,
                             const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const pagestore::LockPageStoreStatus& inStatus) override {
                    if (inStatus == pagestore::kLockPageStoreStatus_Canceled) {
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kCanceled);
                        return;
                    }
                    if (inStatus != pagestore::kLockPageStoreStatus_Success) {
                        NOTIFY_ERROR(h_, "CommitDataStorePageStoreChanges: LockPageStore failed.");
                        mCompletion->Call(h_, pagestore::CommitPageStoreChangesResult::kError);
                        return;
                    }
                    
                    auto unlockingCompletion = std::make_shared<UnlockingCompletion>(mPageStore, mCompletion);
                    PerformWork(h_, mPageStore, unlockingCompletion);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::CommitPageStoreChangesCompletionFunctionPtr mCompletion;
            };
            
        } // namespace CommitDataStorePageStoreChanges_Impl
        using namespace CommitDataStorePageStoreChanges_Impl;
        
        //
        void CommitDataStorePageStoreChanges(const HermitPtr& h_,
                                             const pagestore::PageStorePtr& pageStore,
                                             const pagestore::CommitPageStoreChangesCompletionFunctionPtr& completion) {
            auto lockCompletion = std::make_shared<LockCallback>(pageStore, completion);
            pageStore->Lock(h_, lockCompletion);
        }
        
    } // namespace datastorepagestore
} // namespace hermit

