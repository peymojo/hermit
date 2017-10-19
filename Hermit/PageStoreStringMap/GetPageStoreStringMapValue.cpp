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
#include "GetPageStoreStringMapValue.h"

namespace hermit {
	namespace pagestorestringmap {
		
		namespace {
			
			//
			void GetValue(const StringMapPtr& inMap,
						  const std::string& inKey,
						  const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
				auto mapIt = inMap->mEntries.find(inKey);
				if (mapIt == inMap->mEntries.end()) {
					inCompletionFunction->Call(stringmap::kGetStringMapValueStatus_EntryNotFound, "");
					return;
				}
				inCompletionFunction->Call(stringmap::kGetStringMapValueStatus_EntryFound, mapIt->second);
			}
			
			//
			//
			class LoadCallback
			:
			public LoadStringMapPageCompletionFunction
			{
			public:
				//
				//
				LoadCallback(const HermitPtr& h_,
							 const StringMapPagePtr& inMapPage,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mH_(h_),
				mMapPage(inMapPage),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				void Call(const LoadStringMapPageStatus& inStatus, const StringMapPtr& inMap)
				{
					if (inStatus == LoadStringMapPageStatus::kCanceled) {
						mCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Canceled, "");
						return;
					}
					if (inStatus == LoadStringMapPageStatus::kPageNotFound) {
						NOTIFY_ERROR(mH_, "GetPageStoreStringMapValue: kPageNotFound, key:", mKey);
						mCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Error, "");
						return;
					}
					if (inStatus != LoadStringMapPageStatus::kSuccess) {
						NOTIFY_ERROR(mH_, "GetPageStoreStringMapValue: LoadStringMapPage failed, key:", mKey);
						mCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Error, "");
						return;
					}
					
					mMapPage->mMap = inMap;
					GetValue(inMap, mKey, mCompletionFunction);
				}
				
				//
				//
				HermitPtr mH_;
				StringMapPagePtr mMapPage;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			//
			void PerformWork(const HermitPtr& h_,
							 const stringmap::StringMapPtr& inMap,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction)
			{
				PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inMap);
				
				ThreadLockScope lock(stringMap.mLock);
				
				if (stringMap.mPages.empty())
				{
					inCompletionFunction->Call(stringmap::kGetStringMapValueStatus_EntryNotFound, "");
					return;
				}
				
				auto pageIt = stringMap.mPages.lower_bound(inKey);
				if (pageIt != stringMap.mPages.begin())
				{
					pageIt--;
				}
				
				if (pageIt->second->mMap == nullptr)
				{
					auto completion = std::make_shared<LoadCallback>(h_, pageIt->second, inKey, inCompletionFunction);
					stringMap.LoadPage(h_, pageIt->second->mKey, completion);
					return;
				}
				
				GetValue(pageIt->second->mMap, inKey, inCompletionFunction);
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
				InitCallback(const HermitPtr& h_,
							 const stringmap::StringMapPtr& inStringMap,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void Call(const HermitPtr& h_, const InitPageStoreStringMapStatus& inStatus) override
				{
					if (inStatus == kInitPageStoreStringMapStatus_Canceled)
					{
						mCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Canceled, "");
						return;
					}
					if (inStatus != kInitPageStoreStringMapStatus_Success)
					{
						NOTIFY_ERROR(mH_, "GetPageStoreStringMapValue: Init failed.");
						mCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Error, "");
						return;
					}
					PerformWork(mH_, mStringMap, mKey, mCompletionFunction);
				}
				
				//
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			//
			class Task
			:
			public AsyncTask
			{
			public:
				//
				//
				Task(const HermitPtr& h_,
					 const stringmap::StringMapPtr& inStringMap,
					 const std::string& inKey,
					 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				virtual void PerformTask(const int32_t& inTaskID) {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					
					auto completion = std::make_shared<InitCallback>(mH_, mStringMap, mKey, mCompletionFunction);
					stringMap.Init(mH_, completion);
				}
				
				//
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			//
			class CompletionProxy
			:
			public stringmap::GetStringMapValueCompletionFunction
			{
			public:
				//
				//
				CompletionProxy(
								const stringmap::StringMapPtr& inStringMap,
								const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				void Call(const stringmap::GetStringMapValueStatus& inStatus, const std::string& inValue)
				{
					mCompletionFunction->Call(inStatus, inValue);
					
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					stringMap.TaskComplete();
				}
				
				//
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		//
		void GetPageStoreStringMapValue(const HermitPtr& h_,
										const stringmap::StringMapPtr& inStringMap,
										const std::string& inKey,
										const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction)
		{
			auto completion = std::make_shared<CompletionProxy>(inStringMap, inCompletionFunction);
			auto task = std::make_shared<Task>(h_, inStringMap, inKey, completion);
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			if (!stringMap.QueueTask(task)) {
				NOTIFY_ERROR(h_, "stringMap.QueueTask failed");
				inCompletionFunction->Call(stringmap::kGetStringMapValueStatus_Error, "");
			}
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
