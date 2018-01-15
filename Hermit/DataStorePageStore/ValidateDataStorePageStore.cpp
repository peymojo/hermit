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
#include "DataStorePageStore.h"
#include "ValidateDataStorePageStore.h"

namespace hermit {
    namespace datastorepagestore {
        namespace ValidateDataStorePageStore_Impl {
            
            //
            class PageValidator;
            typedef std::shared_ptr<PageValidator> PageValidatorPtr;

            //
            class PageValidator : public std::enable_shared_from_this<PageValidator> {
            public:
                //
                PageValidator(const pagestore::PageStorePtr& pageStore,
                              const pagestore::ValidatePageStoreCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                void ValidatePages(const HermitPtr& h_) {
                    if (CHECK_FOR_ABORT(h_)) {
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kCanceled);
                        return;
                    }
                    
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    if (!pageStore.mPath->AppendPathComponent(h_, "pages", mPagesPath)) {
                        NOTIFY_ERROR(h_, "ValidatePages: AppendToDataPath failed for pages path.");
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kError);
                        return;
                    }
                    mPageIterator = pageStore.mPageTable.begin();
                    ValidateNextPage(h_);
                }
                
                //
                class ExistsCompletion : public datastore::ItemExistsInDataStoreCompletion {
                public:
                    //
                    ExistsCompletion(const PageValidatorPtr& validator, const std::string& pageFileName) :
                    mValidator(validator),
                    mPageFileName(pageFileName) {
                    }
                    
                    //
                    virtual void Call(const HermitPtr& h_,
                                      const datastore::ItemExistsInDataStoreResult& result,
                                      const bool& exists) override {
                        mValidator->ItemExistsCompletion(h_, mPageFileName, result, exists);
                    }
                    
                    //
                    PageValidatorPtr mValidator;
                    std::string mPageFileName;
                };
                
                //
                void ItemExistsCompletion(const HermitPtr& h_,
                                          const std::string& pageFileName,
                                          const datastore::ItemExistsInDataStoreResult& result,
                                          const bool& exists) {
                    if (result == datastore::ItemExistsInDataStoreResult::kCanceled) {
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kCanceled);
                        return;
                    }
                    if (result != datastore::ItemExistsInDataStoreResult::kSuccess) {
                        NOTIFY_ERROR(h_, "ItemExistsInDataStore failed, continuing, pageFileName:", pageFileName);
                    }
                    else if (!exists) {
                        NOTIFY_ERROR(h_, "ValidatePages: Page missing, continuing, pageFileName:", pageFileName);
                    }
                    ValidateNextPage(h_);
                }
                
                //
                void ValidateNextPage(const HermitPtr& h_) {
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    if (mPageIterator == pageStore.mPageTable.end()) {
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kSuccess);
                        return;
                    }
                    std::string pageFileName = mPageIterator->second;
                    mPageIterator++;
                        
                    datastore::DataPathPtr pagePath;
                    bool success = false;
                    if (pageFileName[0] == '#') {
                        // transitional page store without "pages" subdir for older pages
                        success = pageStore.mPath->AppendPathComponent(h_, pageFileName.c_str() + 1, pagePath);
                    }
                    else {
                        success = mPagesPath->AppendPathComponent(h_, pageFileName, pagePath);
                    }
                        
                    if (!success) {
                        NOTIFY_ERROR(h_, "ValidatePages: AppendPathComponent failed, continuing, pageFileName:", pageFileName);
                        ValidateNextPage(h_);
                        return;
                    }

                    auto existsCompletion = std::make_shared<ExistsCompletion>(shared_from_this(), pageFileName);
                    pageStore.mDataStore->ItemExists(h_, pagePath, existsCompletion);
                }
            
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::ValidatePageStoreCompletionFunctionPtr mCompletion;
                datastore::DataPathPtr mPagesPath;
                DataStorePageStore::PageMap::const_iterator mPageIterator;
            };
            
            //
            class TableLoaded : public ReadPageTableCompletionFunction {
            public:
                //
                TableLoaded(const pagestore::PageStorePtr& pageStore,
                            const pagestore::ValidatePageStoreCompletionFunctionPtr& completion) :
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const ReadPageTableResult& inResult) override {
                    if (inResult == ReadPageTableResult::kCanceled) {
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kCanceled);
                        return;
                    }
                    if (inResult != ReadPageTableResult::kSuccess) {
                        NOTIFY_ERROR(h_, "ReadPageTable failed");
                        mCompletion->Call(h_, pagestore::ValidatePageStoreResult::kError);
                        return;
                    }
                    auto validator = std::make_shared<PageValidator>(mPageStore, mCompletion);
                    validator->ValidatePages(h_);
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::ValidatePageStoreCompletionFunctionPtr mCompletion;
            };
            
            //
            class Task : public AsyncTask {
            public:
                //
                Task(const HermitPtr& h_,
                     const pagestore::PageStorePtr& pageStore,
                     const pagestore::ValidatePageStoreCompletionFunctionPtr& completion) :
                mH_(h_),
                mPageStore(pageStore),
                mCompletion(completion) {
                }
                
                //
                void PerformTask(const int32_t& inTaskID) {
                    if (CHECK_FOR_ABORT(mH_)) {
                        mCompletion->Call(mH_, pagestore::ValidatePageStoreResult::kCanceled);
                        return;
                    }
                    
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    if (!pageStore.mPageTableLoaded) {
                        auto completion = std::make_shared<TableLoaded>(mPageStore, mCompletion);
                        pageStore.ReadPageTable(mH_, completion);
                        return;
                    }
                    auto validator = std::make_shared<PageValidator>(mPageStore, mCompletion);
                    validator->ValidatePages(mH_);
                }
                
                //
                HermitPtr mH_;
                pagestore::PageStorePtr mPageStore;
                pagestore::ValidatePageStoreCompletionFunctionPtr mCompletion;
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
                virtual void Call(const HermitPtr& h_, const pagestore::ValidatePageStoreResult& inResult) override {
                    mCompletionFunction->Call(h_, inResult);
                    DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*mPageStore);
                    pageStore.TaskComplete();
                }
                
                //
                pagestore::PageStorePtr mPageStore;
                pagestore::ValidatePageStoreCompletionFunctionPtr mCompletionFunction;
            };
            
        } // namespace ValidateDataStorePageStore_Impl
        using namespace ValidateDataStorePageStore_Impl;
        
        //
        void ValidateDataStorePageStore(const HermitPtr& h_,
                                        const pagestore::PageStorePtr& inPageStore,
                                        const pagestore::ValidatePageStoreCompletionFunctionPtr& inCompletionFunction) {
            DataStorePageStore& pageStore = static_cast<DataStorePageStore&>(*inPageStore);
            auto proxy = std::make_shared<CompletionProxy>(inPageStore, inCompletionFunction);
            auto task = std::make_shared<Task>(h_, inPageStore, proxy);
            if (!pageStore.QueueTask(task)) {
                NOTIFY_ERROR(h_, "pageStore.QueueTask failed");
                inCompletionFunction->Call(h_, pagestore::ValidatePageStoreResult::kError);
            }
        }
        
    } // namespace datastorepagestore
} // namespace hermit

