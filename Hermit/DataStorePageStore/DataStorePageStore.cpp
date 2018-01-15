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

#include "Hermit/DataStore/DataPath.h"
#include "Hermit/Foundation/Notification.h"
#include "Hermit/JSON/EnumerateRootJSONValuesCallback.h"
#include "Hermit/JSON/JSONToDataValue.h"
#include "Hermit/Value/Value.h"
#include "CommitDataStorePageStoreChanges.h"
#include "DataStorePageStore.h"
#include "EnumerateDataStorePageStorePages.h"
#include "LockDataStorePageStore.h"
#include "ReadDataStorePageStorePage.h"
#include "UnlockDataStorePageStore.h"
#include "ValidateDataStorePageStore.h"
#include "WriteDataStorePageStorePage.h"

namespace hermit {
    namespace datastorepagestore {
        namespace DataStorePageStore_Impl {
            
            //
            typedef std::map<std::string, value::ValuePtr> ValueMap;
            typedef std::vector<value::ValuePtr> ValueVector;
            
            //
            void ProcessPageTable(const HermitPtr& h_,
                                  const pagestore::PageStorePtr& pageStore,
                                  DataStorePageStore::PageMap& pageTable,
                                  const ReadPageTableCompletionFunctionPtr& completion) {
                DataStorePageStore& dataStorePageStore = static_cast<DataStorePageStore&>(*pageStore);
                dataStorePageStore.mPageTable.swap(pageTable);
                dataStorePageStore.mPageTableLoaded = true;
                completion->Call(h_, ReadPageTableResult::kSuccess);
            }
            
            //
            class ListCallback : public datastore::ListDataStoreContentsItemCallback {
            public:
                //
                ListCallback(bool legacy) :
                mLegacy(legacy) {
                }
                
                //
                virtual bool OnOneItem(const HermitPtr& h_, const datastore::DataPathPtr& parentPath, const std::string& itemName) {
                    std::string name(itemName);
                    std::string::size_type dotPos = name.find(".page");
                    if (dotPos != std::string::npos) {
                        std::string key(name.substr(0, dotPos));
                        //    legacy pages were not in a "pages" subfolder
                        if (mLegacy) {
                            name.insert(0, "#");
                        }
                        mPages.insert(DataStorePageStore::PageMap::value_type(key, name));
                    }
                    return true;
                }
                
                //
                DataStorePageStore::PageMap mPages;
                bool mLegacy;
            };
            typedef std::shared_ptr<ListCallback> ListCallbackPtr;
            
            //
            class ListCompletion : public datastore::ListDataStoreContentsCompletion {
            public:
                //
                ListCompletion(const pagestore::PageStorePtr& pageStore,
                               const datastore::DataStorePtr& dataStore,
                               const datastore::DataPathPtr& path,
                               const ListCallbackPtr& listCallback,
                               bool legacy,
                               const ReadPageTableCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mDataStore(dataStore),
                mPath(path),
                mListCallback(listCallback),
                mLegacy(legacy),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const datastore::ListDataStoreContentsResult& result) override {
                    if (result == datastore::ListDataStoreContentsResult::kCanceled) {
                        mCompletion->Call(h_, ReadPageTableResult::kCanceled);
                        return;
                    }
                    if (result == datastore::ListDataStoreContentsResult::kSuccess) {
                        if (!mListCallback->mPages.empty() || mLegacy) {
                            ProcessPageTable(h_, mPageStore, mListCallback->mPages, mCompletion);
                            return;
                        }
                        
                        auto listCallback = std::make_shared<ListCallback>(true);
                        auto listCompletion = std::make_shared<ListCompletion>(mPageStore, mDataStore, mPath, listCallback, true, mCompletion);
                        mDataStore->ListContents(h_, mPath, listCallback, listCompletion);
                        return;
                    }
                    
                    NOTIFY_ERROR(h_, "ListDataStoreContents failed.");
                    mCompletion->Call(h_, ReadPageTableResult::kError);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                datastore::DataStorePtr mDataStore;
                datastore::DataPathPtr mPath;
                ListCallbackPtr mListCallback;
                bool mLegacy;
                ReadPageTableCompletionFunctionPtr mCompletion;
            };
            
            //
            class PageVisitor : public value::ArrayValuePtrVisitor {
            public:
                //
                PageVisitor(DataStorePageStore::PageMap& inPages) :
                mPages(inPages),
                mErrorOccurred(false) {
                }
                
                //
                virtual bool VisitValue(const HermitPtr& h_, const value::ValuePtr& inValue) override {
                    if (inValue->GetDataType() != value::DataType::kObject) {
                        NOTIFY_ERROR(h_, "PageVisitor: inValue->GetDataType() != DataType::kObject");
                        mErrorOccurred = true;
                        return false;
                    }
                    value::ObjectValuePtr childObject = std::static_pointer_cast<value::ObjectValue>(inValue);
                    
                    std::string key;
                    if (!value::GetStringValue(h_, childObject, "key", true, key)) {
                        NOTIFY_ERROR(h_, "PageVisitor: GetStringValue(key) failed?");
                        mErrorOccurred = true;
                        return false;
                    }
                    
                    std::string name;
                    if (!value::GetStringValue(h_, childObject, "name", false, name)) {
                        NOTIFY_ERROR(h_, "PageVisitor: GetStringValue(name) failed?");
                        mErrorOccurred = true;
                        return false;
                    }
                    
                    mPages.insert(DataStorePageStore::PageMap::value_type(key, name));
                    return true;
                }
                
                //
                DataStorePageStore::PageMap& mPages;
                bool mErrorOccurred;
            };
            
            //
            class CompletionBlock : public datastore::LoadDataStoreDataCompletionBlock {
            public:
                //
                CompletionBlock(const pagestore::PageStorePtr& pageStore,
                                const datastore::DataStorePtr& dataStore,
                                const datastore::DataPathPtr& path,
                                const datastore::LoadDataStoreDataDataPtr& data,
                                const ReadPageTableCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mDataStore(dataStore),
                mPath(path),
                mData(data),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const datastore::LoadDataStoreDataResult& result) override {
                    if (result == datastore::LoadDataStoreDataResult::kCanceled) {
                        mCompletion->Call(h_, ReadPageTableResult::kCanceled);
                        return;
                    }
                    
                    if (result == datastore::LoadDataStoreDataResult::kItemNotFound) {
                        datastore::DataPathPtr pagesPath;
                        if (!mPath->AppendPathComponent(h_, "pages", pagesPath)) {
                            NOTIFY_ERROR(h_, "AppendToDataPath failed for pages path.");
                            mCompletion->Call(h_, ReadPageTableResult::kError);
                            return;
                        }
                        
                        auto listCallback = std::make_shared<ListCallback>(false);
                        auto listCompletion = std::make_shared<ListCompletion>(mPageStore, mDataStore, mPath, listCallback, false, mCompletion);
                        mDataStore->ListContents(h_, pagesPath, listCallback, listCompletion);
                        return;
                    }
                    if (result != datastore::LoadDataStoreDataResult::kSuccess) {
                        NOTIFY_ERROR(h_, "LoadDataStoreData failed for index.");
                        mCompletion->Call(h_, ReadPageTableResult::kError);
                        return;
                    }
                    
                    DataStorePageStore::PageMap pageTable;
                    value::ValuePtr values;
                    uint64_t bytesConsumed = 0;
                    if (!json::JSONToDataValue(h_, mData->mData, mData->mData.size(), values, bytesConsumed)) {
                        NOTIFY_ERROR(h_, "JSONToDataValue failed for index.");
                        mCompletion->Call(h_, ReadPageTableResult::kError);
                        return;
                    }
                    
                    value::ObjectValue& indexValues = static_cast<value::ObjectValue&>(*values);
                    value::ValuePtr pagesValue = indexValues.GetItem("pages");
                    if (pagesValue == nullptr) {
                        NOTIFY_ERROR(h_, "pages value missing from index.json");
                        mCompletion->Call(h_, ReadPageTableResult::kError);
                        return;
                    }
                    if (pagesValue->GetDataType() != value::DataType::kArray) {
                        NOTIFY_ERROR(h_, "pages value not an array");
                        mCompletion->Call(h_, ReadPageTableResult::kError);
                        return;
                    }
                    value::ArrayValuePtr pagesArray = std::static_pointer_cast<value::ArrayValue>(pagesValue);
                    
                    PageVisitor pageVisitor(pageTable);
                    pagesArray->VisitItemPtrs(h_, pageVisitor);
                    if (pageVisitor.mErrorOccurred) {
                        NOTIFY_ERROR(h_, "pageVisitor");
                        mCompletion->Call(h_, ReadPageTableResult::kError);
                        return;
                    }
                    
                    ProcessPageTable(h_, mPageStore, pageTable, mCompletion);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                datastore::DataStorePtr mDataStore;
                datastore::DataPathPtr mPath;
                datastore::LoadDataStoreDataDataPtr mData;
                ReadPageTableCompletionFunctionPtr mCompletion;
            };
            
            //
            void PerformReadTableWork(const HermitPtr& h_,
                                      const pagestore::PageStorePtr& inPageStore,
                                      const ReadPageTableCompletionFunctionPtr& inCompletionFunction) {
                
                DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
                if (pageStore.mPageTableLoaded) {
                    inCompletionFunction->Call(h_, ReadPageTableResult::kSuccess);
                    return;
                }
                
                
                datastore::DataStorePtr dataStore = pageStore.mDataStore;
                datastore::DataPathPtr path = pageStore.mPath;
                
                if (CHECK_FOR_ABORT(h_)) {
                    inCompletionFunction->Call(h_, ReadPageTableResult::kCanceled);
                    return;
                }
                
                std::string indexName("index.json");
                datastore::DataPathPtr indexPath;
                if (!path->AppendPathComponent(h_, indexName, indexPath)) {
                    NOTIFY_ERROR(h_, "AppendToDataPath failed for indexName:", indexName);
                    inCompletionFunction->Call(h_, ReadPageTableResult::kError);
                    return;
                }
                
                auto dataBlock = std::make_shared<datastore::LoadDataStoreDataData>();
                auto completion = std::make_shared<CompletionBlock>(inPageStore, dataStore, path, dataBlock, inCompletionFunction);
                auto encryptionSetting = datastore::EncryptionSetting::kDefault;
                dataStore->LoadData(h_, indexPath, encryptionSetting, dataBlock, completion);
            }
            
            //
            class ReadTableTask : public AsyncTask {
            public:
                //
                ReadTableTask(const HermitPtr& h_,
                              const pagestore::PageStorePtr& inPageStore,
                              const ReadPageTableCompletionFunctionPtr& inCompletionFunction) :
                mH_(h_),
                mPageStore(inPageStore),
                mCompletionFunction(inCompletionFunction) {
                }
                
                //
                void PerformTask(const int32_t& inTaskID) {
                    PerformReadTableWork(mH_, mPageStore, mCompletionFunction);
                }
                
                //
                HermitPtr mH_;
                pagestore::PageStorePtr mPageStore;
                ReadPageTableCompletionFunctionPtr mCompletionFunction;
            };
            
        } // namespace DataStorePageStore_Impl
        using namespace DataStorePageStore_Impl;
        
        //
        DataStorePageStore::DataStorePageStore(const datastore::DataStorePtr& inDataStore, const datastore::DataPathPtr& inPath) :
        mDataStore(inDataStore),
        mPath(inPath),
        mPageTableLoaded(false) {
        }
        
        //
        void DataStorePageStore::EnumeratePages(const HermitPtr& h_,
                                                const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
                                                const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) {
            EnumerateDataStorePageStorePages(h_,
                                             shared_from_this(),
                                             inEnumerationFunction,
                                             inCompletionFunction);
        }
        
        //
        void DataStorePageStore::ReadPage(const HermitPtr& h_,
                                          const std::string& inPageName,
                                          const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) {
            ReadDataStorePageStorePage(h_, shared_from_this(), inPageName, inCompletionFunction);
        }
        
        //
        void DataStorePageStore::WritePage(const HermitPtr& h_,
                                           const std::string& inPageName,
                                           const DataBuffer& inPageData,
                                           const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) {
            WriteDataStorePageStorePage(h_,
                                        shared_from_this(),
                                        inPageName,
                                        inPageData,
                                        inCompletionFunction);
        }
        
        //
        void DataStorePageStore::Lock(const HermitPtr& h_,
                                      const pagestore::LockPageStoreCompletionFunctionPtr& inCompletionFunction) {
            LockDataStorePageStore(h_, shared_from_this(), inCompletionFunction);
        }
        
        //
        void DataStorePageStore::Unlock(const HermitPtr& h_) {
            UnlockDataStorePageStore(h_, shared_from_this());
        }
        
        //
        void DataStorePageStore::CommitChanges(const HermitPtr& h_,
                                               const pagestore::CommitPageStoreChangesCompletionFunctionPtr& inCompletionFunction) {
            CommitDataStorePageStoreChanges(h_, shared_from_this(), inCompletionFunction);
        }
        
        //
        void DataStorePageStore::Validate(const HermitPtr& h_,
                                          const pagestore::ValidatePageStoreCompletionFunctionPtr& inCompletionFunction) {
            ValidateDataStorePageStore(h_, shared_from_this(), inCompletionFunction);
        }
        
        //
        void DataStorePageStore::ReadPageTable(const HermitPtr& h_, const ReadPageTableCompletionFunctionPtr& inCompletionFunction) {
            auto task = std::make_shared<ReadTableTask>(h_, shared_from_this(), inCompletionFunction);
            if (!QueueAsyncTask(task, 5)) {
                inCompletionFunction->Call(h_, ReadPageTableResult::kError);
            }
        }
        
    } // namespace datastorepagestore
} // namespace hermit

