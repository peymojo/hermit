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
#include "Hermit/Foundation/QueueAsyncTask.h"
#include "CommitPageStoreStringMapChanges.h"
#include "GetPageStoreStringMapValue.h"
#include "LockPageStoreStringMap.h"
#include "PageStoreStringMap.h"
#include "SetPageStoreStringMapValue.h"
#include "UnlockPageStoreStringMap.h"
#include "ValidatePageStoreStringMap.h"

namespace hermit {
	namespace pagestorestringmap {
		
		namespace {
			
			//
			bool ParsePageData(const HermitPtr& h_, const DataBuffer& inPageData, StringMapPtr& outMap) {
				StringMapPtr map(new StringMap());
				
				const char* begin = inPageData.first;
				const char* end = begin + inPageData.second;
				const char* p = begin;
				while (p < end) {
					std::string key(p);
					p += (key.size() + 1);
					if (p >= end) {
						uint64_t offset = p - begin;
						NOTIFY_ERROR(h_, "ParsePageData: Unexpected end at offset:", offset);
						return false;
					}
					std::string value(p);
					if (value.empty()) {
						uint64_t offset = p - begin;
						NOTIFY_ERROR(h_, "ParsePageData: Empty value at offset:", offset);
						return false;
					}
					
					p += (value.size() + 1);
					map->mEntries.insert(StringMap::Map::value_type(key, value));
				}
				outMap = map;
				return true;
			}
			
			//
			class ReadPageCompletion : public pagestore::ReadPageStorePageCompletionFunction {
			public:
				//
				ReadPageCompletion(const HermitPtr& h_,
								   const stringmap::StringMapPtr& inStringMap,
								   const std::string& inKey,
								   const LoadStringMapPageCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				void Call(const pagestore::ReadPageStorePageResult& inResult, const DataBuffer& inPageData) {
					if (inResult == pagestore::kReadPageStorePageResult_Canceled) {
						mCompletionFunction->Call(LoadStringMapPageStatus::kCanceled, nullptr);
						return;
					}
					if (inResult == pagestore::kReadPageStorePageResult_PageNotFound) {
						NOTIFY_ERROR(mH_, "PageStoreStringMap::LoadPage: kReadPageStorePageResult_PageNotFound for page:", mKey);
						mCompletionFunction->Call(LoadStringMapPageStatus::kPageNotFound, nullptr);
						return;
					}
					if (inResult != pagestore::kReadPageStorePageResult_Success) {
						NOTIFY_ERROR(mH_, "PageStoreStringMap::LoadPage: ReadPageStorePage failed for page:", mKey);
						mCompletionFunction->Call(LoadStringMapPageStatus::kError, nullptr);
						return;
					}
					
					StringMapPtr map;
					if (!ParsePageData(mH_, inPageData, map)) {
						NOTIFY_ERROR(mH_, "PageStoreStringMap::LoadPage: ParsePageData failed for page:", mKey);
						mCompletionFunction->Call(LoadStringMapPageStatus::kError, nullptr);
						return;
					}
					mCompletionFunction->Call(LoadStringMapPageStatus::kSuccess, map);
				}
				
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				LoadStringMapPageCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class LoadPageTask : public AsyncTask {
			public:
				//
				LoadPageTask(const HermitPtr& h_,
							 const stringmap::StringMapPtr& inStringMap,
							 const std::string& inKey,
							 const LoadStringMapPageCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				void PerformTask(const int32_t& inTaskID) {
					auto completion = std::make_shared<ReadPageCompletion>(mH_, mStringMap, mKey, mCompletionFunction);
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					stringMap.mPageStore->ReadPage(mH_, mKey, completion);
				}
				
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				LoadStringMapPageCompletionFunctionPtr mCompletionFunction;
			};
			
			
			//
			class PageEnumeration : public pagestore::EnumeratePageStorePagesEnumerationFunction {
			public:
				//
				PageEnumeration(const stringmap::StringMapPtr& inStringMap) :
				mStringMap(inStringMap) {
				}
				
				//
				bool Call(const std::string& inPageName) {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					
					StringMapPagePtr page(new StringMapPage());
					page->mKey = inPageName;
					stringMap.mPages.insert(PageStoreStringMap::PageMap::value_type(inPageName, page));
					return true;
				}
				
				//
				stringmap::StringMapPtr mStringMap;
			};
			
			//
			class PageCompletion : public pagestore::EnumeratePageStorePagesCompletionFunction {
			public:
				//
				PageCompletion(const stringmap::StringMapPtr& inStringMap,
							   const InitPageStoreStringMapCompletionFunctionPtr& inCompletionFunction) :
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const pagestore::EnumeratePageStorePagesResult& inResult) override {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					
					if (inResult == pagestore::kEnumeratePageStorePagesResult_Canceled) {
						stringMap.mInitStatus = kInitPageStoreStringMapStatus_Canceled;
						mCompletionFunction->Call(h_, stringMap.mInitStatus);
						return;
					}
					if (inResult != pagestore::kEnumeratePageStorePagesResult_Success) {
						NOTIFY_ERROR(h_, "PageStoreStringMap::Init: EnumeratePageStorePages failed.");
						stringMap.mInitStatus = kInitPageStoreStringMapStatus_Error;
						mCompletionFunction->Call(h_, stringMap.mInitStatus);
						return;
					}
					stringMap.mInitStatus = kInitPageStoreStringMapStatus_Success;
					mCompletionFunction->Call(h_, stringMap.mInitStatus);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				InitPageStoreStringMapCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class InitTask : public AsyncTask {
			public:
				//
				InitTask(const HermitPtr& h_,
						 const stringmap::StringMapPtr& inStringMap,
						 const InitPageStoreStringMapCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				void PerformTask(const int32_t& inTaskID) {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					if (stringMap.mInitStatus != kInitPageStoreStringMapStatus_Unknown) {
						mCompletionFunction->Call(mH_, stringMap.mInitStatus);
						return;
					}
					
					auto enumeration = std::make_shared<PageEnumeration>(mStringMap);
					auto completion = std::make_shared<PageCompletion>(mStringMap, mCompletionFunction);
					stringMap.mPageStore->EnumeratePages(mH_, enumeration, completion);
				}
				
				//
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				InitPageStoreStringMapCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		PageStoreStringMap::PageStoreStringMap(const pagestore::PageStorePtr& inPageStore) :
		mPageStore(inPageStore),
		mInitStatus(kInitPageStoreStringMapStatus_Unknown) {
		}
		
		//
		void PageStoreStringMap::Init(const HermitPtr& h_,
									  const InitPageStoreStringMapCompletionFunctionPtr& inCompletionFunction) {
			auto task = std::make_shared<InitTask>(h_, shared_from_this(), inCompletionFunction);
			if (!QueueAsyncTask(task, 20)) {
				NOTIFY_ERROR(h_, "QueueAsyncTask failed");
				inCompletionFunction->Call(h_, kInitPageStoreStringMapStatus_Error);
			}
		}
		
		//
		void PageStoreStringMap::LoadPage(const HermitPtr& h_,
										  const std::string& inKey,
										  const LoadStringMapPageCompletionFunctionPtr& inCompletionFunction) {
			auto task = std::make_shared<LoadPageTask>(h_, shared_from_this(), inKey, inCompletionFunction);
			if (!QueueAsyncTask(task, 20)) {
				NOTIFY_ERROR(h_, "QueueAsyncTask failed");
				inCompletionFunction->Call(LoadStringMapPageStatus::kError, nullptr);
			}
		}
		
		//
		void PageStoreStringMap::GetValue(const HermitPtr& h_,
										  const std::string& inKey,
										  const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
			GetPageStoreStringMapValue(h_, shared_from_this(), inKey, inCompletionFunction);
		}
		
		//
		void PageStoreStringMap::SetValue(const HermitPtr& h_,
										  const std::string& inKey,
										  const std::string& inValue,
										  const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
			SetPageStoreStringMapValue(h_, shared_from_this(), inKey, inValue, inCompletionFunction);
		}
		
		//
		void PageStoreStringMap::Lock(const HermitPtr& h_,
									  const stringmap::LockStringMapCompletionFunctionPtr& inCompletionFunction) {
			LockPageStoreStringMap(h_, shared_from_this(), inCompletionFunction);
		}
		
		//
		void PageStoreStringMap::Unlock(const HermitPtr& h_) {
			UnlockPageStoreStringMap(h_, shared_from_this());
		}
		
		//
		void PageStoreStringMap::CommitChanges(const HermitPtr& h_,
											   const stringmap::CommitStringMapChangesCompletionFunctionPtr& inCompletionFunction) {
			CommitPageStoreStringMapChanges(h_, shared_from_this(), inCompletionFunction);
		}
		
		//
		void PageStoreStringMap::Validate(const HermitPtr& h_,
										  const stringmap::ValidateStringMapCompletionFunctionPtr& inCompletionFunction) {
			ValidatePageStoreStringMap(h_, shared_from_this(), inCompletionFunction);
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
