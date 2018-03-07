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
#include "PageStoreStringMap.h"
#include "CommitPageStoreStringMapChanges.h"

namespace hermit {
	namespace pagestorestringmap {
		namespace CommitPageStoreStringMapChanges_Impl {
			
			//
			const size_t kMaxPageSize = 10000;
			
			//
			class CommitCallback : public pagestore::CommitPageStoreChangesCompletionFunction {
			public:
				//
				CommitCallback(const stringmap::StringMapPtr& stringMap,
							   const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const pagestore::CommitPageStoreChangesResult& inResult) override {
					if (inResult == pagestore::CommitPageStoreChangesResult::kCanceled) {
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
						return;
					}
					if (inResult != pagestore::CommitPageStoreChangesResult::kSuccess) {
						NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: CommitPageStoreChanges failed.");
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
						return;
					}
					
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					auto pageEnd = stringMap.mPages.end();
					for (auto pageIt = stringMap.mPages.begin(); pageIt != pageEnd; ++pageIt) {
						pageIt->second->mDirty = false;
					}
					
					mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kSuccess);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::CommitStringMapChangesCompletionFunctionPtr mCompletion;
			};
			
			//
			class TaskAggregator {
			public:
				//
				TaskAggregator(const stringmap::StringMapPtr& stringMap,
							   const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion),
				mPrimaryTaskFailed(false),
				mAllTasksAdded(false),
				mOutstandingTasks(0),
				mAllTasksReportedComplete(false),
				mAtLeastOneTaskFailed(false),
				mAtLeastOneTaskCanceled(false) {
				}
				
				//
				void AllTasksAdded(const hermit::HermitPtr& h_) {
					{
						ThreadLockScope lock(mLock);
						mAllTasksAdded = true;
					}
					
					if (AllTasksAreComplete()) {
						AllTasksComplete(h_);
					}
				}
				
				//
				void PrimaryTaskFailed(const hermit::HermitPtr& h_) {
					{
						ThreadLockScope lock(mLock);
						mPrimaryTaskFailed = true;
					}
					
					if (AllTasksAreComplete()) {
						AllTasksComplete(h_);
					}
				}
				
				//
				void AddTask() {
					mOutstandingTasks++;
				}
				
				//
				void TaskComplete(const hermit::HermitPtr& h_, const pagestore::WritePageStorePageResult& result) {
					mOutstandingTasks--;
					if (result == pagestore::WritePageStorePageResult::kCanceled) {
						mAtLeastOneTaskCanceled = true;
					}
					else if (result != pagestore::WritePageStorePageResult::kSuccess) {
						NOTIFY_ERROR(h_, "TaskAggregator: task failed.");
						mAtLeastOneTaskFailed = true;
					}
					
					if (AllTasksAreComplete()) {
						AllTasksComplete(h_);
					}
				}
				
				//
				void AllTasksComplete(const hermit::HermitPtr& h_) {
					if (mAtLeastOneTaskCanceled) {
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
						return;
					}
					if (mPrimaryTaskFailed) {
						NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: mPrimaryTaskFailed.");
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
						return;
					}
					if (mAtLeastOneTaskFailed) {
						NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: AtLeastOneTaskFailed.");
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
						return;
					}
					
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					auto completion = std::make_shared<CommitCallback>(mStringMap, mCompletion);
					stringMap.mPageStore->CommitChanges(h_, completion);
				}
				
				//
				bool AllTasksAreComplete() {
					ThreadLockScope lock(mLock);
					if (!mAllTasksAdded) {
						return false;
					}
					if (mOutstandingTasks > 0) {
						return false;
					}
					if (!mAllTasksReportedComplete) {
						mAllTasksReportedComplete = true;
						return true;
					}
					return false;
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::CommitStringMapChangesCompletionFunctionPtr mCompletion;
				bool mPrimaryTaskFailed;
				ThreadLock mLock;
				std::atomic<int> mOutstandingTasks;
				std::atomic<bool> mAllTasksAdded;
				std::atomic<bool> mAllTasksReportedComplete;
				std::atomic<bool> mAtLeastOneTaskFailed;
				std::atomic<bool> mAtLeastOneTaskCanceled;
			};
			typedef std::shared_ptr<TaskAggregator> TaskAggregatorPtr;
			
			//
			class CompletionFunction : public pagestore::WritePageStorePageCompletionFunction {
			public:
				//
				CompletionFunction(const TaskAggregatorPtr& inTaskAggregator) : mTaskAggregator(inTaskAggregator) {
				}
				
				//
				virtual void Call(const hermit::HermitPtr& h_, const pagestore::WritePageStorePageResult& result) override {
					mTaskAggregator->TaskComplete(h_, result);
				}
				
				//
				TaskAggregatorPtr mTaskAggregator;
			};
			
			//
			bool StringMapToData(const StringMap::Map& entries, std::string& outData) {
				std::string result;
				for (auto it = begin(entries); it != end(entries); ++it) {
					result.append(it->first);
					result.push_back(0);
					result.append(it->second);
					result.push_back(0);
				}
				outData = result;
				return true;
			}
			
			//
			std::string SimplifyKey(const std::string& key, const std::string& priorKey) {
				std::string simpleKey;
				for (std::string::size_type n = 0; n < key.size(); ++n) {
					simpleKey += key[n];
					if (simpleKey > priorKey) {
						return simpleKey;
					}
				}
				return key;
			}
			
			//
			bool SplitPages(PageStoreStringMap::PageMap& inPageMap) {
				PageStoreStringMap::PageMap pageMap(inPageMap);
				
				bool madeAChange = false;
				auto pageEnd = pageMap.end();
				for (auto pageIt = pageMap.begin(); pageIt != pageEnd; ++pageIt) {
					if (pageIt->second->mMap != nullptr) {
						StringMap::Map::size_type size = pageIt->second->mMap->mEntries.size();
						if (size > kMaxPageSize) {
							auto midPointIt = pageIt->second->mMap->mEntries.begin();
							for (StringMap::Map::size_type t = 0; t < size / 2; ++t) {
								midPointIt++;
							}
							
							StringMapPtr newMap(new StringMap());
							newMap->mEntries.insert(midPointIt, pageIt->second->mMap->mEntries.end());
							
							StringMapPagePtr newPage(new StringMapPage());
							
							auto priorIt = midPointIt;
							--priorIt;
							
							newPage->mKey = SimplifyKey(midPointIt->first, priorIt->first);
							newPage->mMap = newMap;
							newPage->mDirty = true;
							
							pageIt->second->mMap->mEntries.erase(midPointIt, pageIt->second->mMap->mEntries.end());
							pageIt->second->mDirty = true;
							
							inPageMap.insert(PageStoreStringMap::PageMap::value_type(newPage->mKey, newPage));
							madeAChange = true;
						}
					}
				}
				return madeAChange;
			}
			
			//
			void PerformWork(const hermit::HermitPtr& h_,
							 const stringmap::StringMapPtr& stringMap,
							 const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) {
				PageStoreStringMap& pageStoreStringMap = static_cast<PageStoreStringMap&>(*stringMap);
				
				ThreadLockScope lock(pageStoreStringMap.mLock);
				
				//	Repeated iterations may be required to get pages down to desired size.
				while (SplitPages(pageStoreStringMap.mPages));
				
				TaskAggregatorPtr taskAggregator(new TaskAggregator(stringMap, completion));
				auto pageEnd = pageStoreStringMap.mPages.end();
				for (auto pageIt = pageStoreStringMap.mPages.begin(); pageIt != pageEnd; ++pageIt) {
					if (pageIt->second->mDirty) {
						std::string pageData;
						if (!StringMapToData(pageIt->second->mMap->mEntries, pageData)) {
							NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: StringMapToData failed for key:", pageIt->second->mKey);
							taskAggregator->PrimaryTaskFailed(h_);
							break;
						}
						
						auto completion = std::make_shared<CompletionFunction>(taskAggregator);
						pageStoreStringMap.mPageStore->WritePage(h_,
																 pageIt->second->mKey,
																 DataBuffer(pageData.data(), pageData.size()),
																 completion);
					}
				}
				taskAggregator->AllTasksAdded(h_);
			}
			
			//
			class InitCallback : public InitPageStoreStringMapCompletionFunction {
			public:
				//
				InitCallback(const stringmap::StringMapPtr& stringMap,
							 const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const InitPageStoreStringMapStatus& inStatus) override {
					if (inStatus == kInitPageStoreStringMapStatus_Canceled) {
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
						return;
					}
					if (inStatus != kInitPageStoreStringMapStatus_Success) {
						NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: Init failed.");
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
						return;
					}
					PerformWork(h_, mStringMap, mCompletion);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::CommitStringMapChangesCompletionFunctionPtr mCompletion;
			};
			
			//
			class UnlockingCompletion : public stringmap::CommitStringMapChangesCompletionFunction {
			public:
				//
				UnlockingCompletion(const stringmap::StringMapPtr& stringMap,
									const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const stringmap::CommitStringMapChangesResult& inResult) override {
					mStringMap->Unlock(h_);
					mCompletion->Call(h_, inResult);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::CommitStringMapChangesCompletionFunctionPtr mCompletion;
			};
			
			//
			class LockCallback : public stringmap::LockStringMapCompletionFunction {
			public:
				//
				LockCallback(const stringmap::StringMapPtr& stringMap,
							 const stringmap::CommitStringMapChangesCompletionFunctionPtr& completion) :
				mStringMap(stringMap),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const stringmap::LockStringMapResult& inResult) {
					if (inResult == stringmap::LockStringMapResult::kCanceled) {
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
						return;
					}
					if (inResult != stringmap::LockStringMapResult::kSuccess) {
						NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: LockStringMap failed.");
						mCompletion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
						return;
					}
					
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					auto unlockingCompletion = std::make_shared<UnlockingCompletion>(mStringMap, mCompletion);
					auto completion = std::make_shared<InitCallback>(mStringMap, unlockingCompletion);
					stringMap.Init(h_, completion);
				}
				
				//
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::CommitStringMapChangesCompletionFunctionPtr mCompletion;
			};
			
		} // namespace CommitPageStoreStringMapChanges_Impl
		using namespace CommitPageStoreStringMapChanges_Impl;
		
		//
		void CommitPageStoreStringMapChanges(const HermitPtr& h_,
											 const stringmap::StringMapPtr& inStringMap,
											 const stringmap::CommitStringMapChangesCompletionFunctionPtr& inCompletionFunction) {
			auto completion = std::make_shared<LockCallback>(inStringMap, inCompletionFunction);
			inStringMap->Lock(h_, completion);
		}
		
	} // namespace pagestorestringmap
} // namespace hermit

