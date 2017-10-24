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
#include "Hermit/Foundation/ParamBlock.h"
#include "PageStoreStringMap.h"
#include "CommitPageStoreStringMapChanges.h"

namespace hermit {
namespace pagestorestringmap {

namespace {

	//
	//
	const size_t kMaxPageSize = 10000;

	//
	//
	DEFINE_PARAMBLOCK_3(CommitPageStoreStringMapChangesParams,
						HermitPtr, h_,
						stringmap::StringMapPtr, stringMap,
						stringmap::CommitStringMapChangesCompletionFunctionPtr, completion);

	//
	class CommitCallback : public pagestore::CommitPageStoreChangesCompletionFunction {
	public:
		//
		CommitCallback(const CommitPageStoreStringMapChangesParams& inParams) : mParams(inParams) {
		}
		
		//
		virtual void Call(const HermitPtr& h_, const pagestore::CommitPageStoreChangesResult& inResult) override {
			if (inResult == pagestore::CommitPageStoreChangesResult::kCanceled) {
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
				return;
			}
			if (inResult != pagestore::CommitPageStoreChangesResult::kSuccess) {
				NOTIFY_ERROR(h_, "CommitPageStoreStringMapChanges: CommitPageStoreChanges failed.");
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
				return;
			}

			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mParams.stringMap);
			auto pageEnd = stringMap.mPages.end();
			for (auto pageIt = stringMap.mPages.begin(); pageIt != pageEnd; ++pageIt) {
				pageIt->second->mDirty = false;
			}

			mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kSuccess);
		}
		
		//
		CommitPageStoreStringMapChangesParams mParams;
	};
	
	//
	//
	class TaskAggregator {
	public:
		//
		//
		TaskAggregator(
			const CommitPageStoreStringMapChangesParams& inParams)
			:
			mParams(inParams),
			mPrimaryTaskFailed(false),
			mAllTasksAdded(false),
			mOutstandingTasks(0),
			mAllTasksReportedComplete(false),
			mAtLeastOneTaskFailed(false),
			mAtLeastOneTaskCanceled(false)
		{
		}
		
		//
		//
		void AllTasksAdded()
		{
			{
				ThreadLockScope lock(mLock);
				mAllTasksAdded = true;
			}
			
			if (AllTasksAreComplete())
			{
				AllTasksComplete();
			}
		}
		
		//
		//
		void PrimaryTaskFailed()
		{
			{
				ThreadLockScope lock(mLock);
				mPrimaryTaskFailed = true;
			}
			
			if (AllTasksAreComplete())
			{
				AllTasksComplete();
			}
		}
		
		//
		//
		void AddTask() {
			mOutstandingTasks++;
		}
		
		//
		//
		void TaskComplete(const pagestore::WritePageStorePageResult& inStatus)
		{
			mOutstandingTasks--;
			if (inStatus == pagestore::kWritePageStorePageResult_Canceled) {
				mAtLeastOneTaskCanceled = true;
			}
			else if (inStatus != pagestore::kWritePageStorePageResult_Success) {
				NOTIFY_ERROR(mParams.h_, "TaskAggregator: task failed.");
				mAtLeastOneTaskFailed = true;
			}
			
			if (AllTasksAreComplete())
			{
				AllTasksComplete();
			}
		}

		//
		void AllTasksComplete() {
			if (mAtLeastOneTaskCanceled) {
				mParams.completion->Call(mParams.h_, stringmap::CommitStringMapChangesResult::kCanceled);
				return;
			}
			if (mPrimaryTaskFailed) {
				NOTIFY_ERROR(mParams.h_, "CommitPageStoreStringMapChanges: mPrimaryTaskFailed.");
				mParams.completion->Call(mParams.h_, stringmap::CommitStringMapChangesResult::kError);
				return;
			}
			if (mAtLeastOneTaskFailed) {
				NOTIFY_ERROR(mParams.h_, "CommitPageStoreStringMapChanges: AtLeastOneTaskFailed.");
				mParams.completion->Call(mParams.h_, stringmap::CommitStringMapChangesResult::kError);
				return;
			}

			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mParams.stringMap);
			pagestore::CommitPageStoreChangesCompletionFunctionPtr completion(new CommitCallback(mParams));
			stringMap.mPageStore->CommitChanges(mParams.h_, completion);
		}
		
		//
		//
		bool AllTasksAreComplete()
		{
			ThreadLockScope lock(mLock);
			if (!mAllTasksAdded)
			{
				return false;
			}
			if (mOutstandingTasks > 0) {
				return false;
			}
			if (!mAllTasksReportedComplete)
			{
				mAllTasksReportedComplete = true;
				return true;
			}
			return false;
		}
		
		//
		//
		CommitPageStoreStringMapChangesParams mParams;
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
	//
	class CompletionFunction
		:
		public pagestore::WritePageStorePageCompletionFunction
	{
	public:
		//
		//
		CompletionFunction(
			const TaskAggregatorPtr& inTaskAggregator)
			:
			mTaskAggregator(inTaskAggregator)
		{
		}
		
		//
		//
		virtual void Call(const pagestore::WritePageStorePageResult& inStatus) override
		{
			mTaskAggregator->TaskComplete(inStatus);
		}
		
		//
		//
		TaskAggregatorPtr mTaskAggregator;
	};

	//
	//
	bool StringMapToData(
		const StringMap::Map& inEntries,
		std::string& outData)
	{
		std::string result;
		auto it = inEntries.begin();
		while (it != inEntries.end())
		{
			result.append(it->first);
			result.push_back(0);
			result.append(it->second);
			result.push_back(0);
		
			++it;
		}
		outData = result;
		return true;
	}
	
	//
	//
	std::string SimplifyKey(
		const std::string& inKey,
		const std::string& inPriorKey)
	{
		std::string simpleKey;
		for (std::string::size_type n = 0; n < inKey.size(); ++n)
		{
			simpleKey += inKey[n];
			if (simpleKey > inPriorKey)
			{
				return simpleKey;
			}
		}
		return inKey;
	}

	//
	//
	bool SplitPages(
		PageStoreStringMap::PageMap& inPageMap)
	{
		PageStoreStringMap::PageMap pageMap(inPageMap);
	
		bool madeAChange = false;
		auto pageEnd = pageMap.end();
		for (auto pageIt = pageMap.begin(); pageIt != pageEnd; ++pageIt)
		{
			if (pageIt->second->mMap != nullptr)
			{
				StringMap::Map::size_type size = pageIt->second->mMap->mEntries.size();
				if (size > kMaxPageSize)
				{
					auto midPointIt = pageIt->second->mMap->mEntries.begin();
					for (StringMap::Map::size_type t = 0; t < size / 2; ++t)
					{
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
	//
	void PerformWork(
		const CommitPageStoreStringMapChangesParams& inParams)
	{
		PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inParams.stringMap);
		
		ThreadLockScope lock(stringMap.mLock);

		//	Repeated iterations may be required to get pages down to desired size.
		while (SplitPages(stringMap.mPages));

		TaskAggregatorPtr taskAggregator(new TaskAggregator(inParams));
		auto pageEnd = stringMap.mPages.end();
		for (auto pageIt = stringMap.mPages.begin(); pageIt != pageEnd; ++pageIt)
		{
			if (pageIt->second->mDirty)
			{
				std::string pageData;
				if (!StringMapToData(pageIt->second->mMap->mEntries, pageData))
				{
					NOTIFY_ERROR(inParams.h_,
								 "CommitPageStoreStringMapChanges: StringMapToData failed for key:",
								 pageIt->second->mKey);
					taskAggregator->PrimaryTaskFailed();
					break;
				}
				
				auto completion = std::make_shared<CompletionFunction>(taskAggregator);
				stringMap.mPageStore->WritePage(inParams.h_,
												pageIt->second->mKey,
												DataBuffer(pageData.data(), pageData.size()),
												completion);
			}
		}
		taskAggregator->AllTasksAdded();
	}

	//
	//
	class InitCallback
		:
		public InitPageStoreStringMapCompletionFunction
	{
	public:
		//
		//
		InitCallback(
			const CommitPageStoreStringMapChangesParams& inParams)
			:
			mParams(inParams)
		{
		}

		//
		//
		virtual void Call(const HermitPtr& h_, const InitPageStoreStringMapStatus& inStatus) override
		{
			if (inStatus == kInitPageStoreStringMapStatus_Canceled)
			{
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
				return;
			}
			if (inStatus != kInitPageStoreStringMapStatus_Success)
			{
				NOTIFY_ERROR(mParams.h_, "CommitPageStoreStringMapChanges: Init failed.");
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
				return;
			}
			PerformWork(mParams);
		}
		
		//
		//
		CommitPageStoreStringMapChangesParams mParams;
	};

	//
	//
	class UnlockingCompletion
		:
		public stringmap::CommitStringMapChangesCompletionFunction
	{
	public:
		//
		UnlockingCompletion(const CommitPageStoreStringMapChangesParams& inParams)
			:
			mParams(inParams) {
		}
		
		//
		virtual void Call(const HermitPtr& h_, const stringmap::CommitStringMapChangesResult& inResult) override {
			mParams.stringMap->Unlock(h_);
			mParams.completion->Call(h_, inResult);
		}

		//
		CommitPageStoreStringMapChangesParams mParams;
	};
	
	//
	//
	class LockCallback
		:
		public stringmap::LockStringMapCompletionFunction
	{
	public:
		//
		//
		LockCallback(
			const CommitPageStoreStringMapChangesParams& inParams)
			:
			mParams(inParams)
		{
		}
		
		//
		//
		virtual void Call(const HermitPtr& h_, const stringmap::LockStringMapResult& inResult)
		{
			if (inResult == stringmap::LockStringMapResult::kCanceled)
			{
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kCanceled);
				return;
			}
			if (inResult != stringmap::LockStringMapResult::kSuccess)
			{
				NOTIFY_ERROR(mParams.h_, "CommitPageStoreStringMapChanges: LockStringMap failed.");
				mParams.completion->Call(h_, stringmap::CommitStringMapChangesResult::kError);
				return;
			}
			
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mParams.stringMap);

			auto unlockingCompletion = std::make_shared<UnlockingCompletion>(mParams);
			mParams.completion = unlockingCompletion;
			auto completion = std::make_shared<InitCallback>(mParams);
			stringMap.Init(mParams.h_, completion);
		}
		
		//
		//
		CommitPageStoreStringMapChangesParams mParams;
	};

} // private namespace

//
//
void CommitPageStoreStringMapChanges(const HermitPtr& h_,
									 const stringmap::StringMapPtr& inStringMap,
									 const stringmap::CommitStringMapChangesCompletionFunctionPtr& inCompletionFunction) {
	auto params = CommitPageStoreStringMapChangesParams(h_, inStringMap, inCompletionFunction);
	auto completion = std::make_shared<LockCallback>(params);
	inStringMap->Lock(h_, completion);
}

} // namespace pagestorestringmap
} // namespace hermit
