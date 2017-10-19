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
#include "SetPageStoreStringMapValue.h"

namespace hermit {
	namespace pagestorestringmap {
		
		namespace {
			
			//
			void SetValue(const HermitPtr& h_,
						  const StringMapPagePtr& inMapPage,
						  const std::string& inKey,
						  const std::string& inValue,
						  const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
				auto it = inMapPage->mMap->mEntries.find(inKey);
				if (it != inMapPage->mMap->mEntries.end()) {
					it->second = inValue;
				}
				else {
					inMapPage->mMap->mEntries.insert(StringMap::Map::value_type(inKey, inValue));
				}
				inMapPage->mDirty = true;
				
				inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Success);
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
							 const std::string& inValue,
							 const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mH_(h_),
				mMapPage(inMapPage),
				mKey(inKey),
				mValue(inValue),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void Call(const LoadStringMapPageStatus& inStatus, const StringMapPtr& inMap) override
				{
					if (inStatus == LoadStringMapPageStatus::kCanceled) {
						mCompletionFunction->Call(mH_, stringmap::kSetStringMapValueResult_Canceled);
						return;
					}
					if (inStatus == LoadStringMapPageStatus::kPageNotFound) {
						NOTIFY_ERROR(mH_, "SetPageStoreStringMapValue: kPageNotFound, key:", mKey);
						mCompletionFunction->Call(mH_, stringmap::kSetStringMapValueResult_Error);
						return;
					}
					if (inStatus != LoadStringMapPageStatus::kSuccess) {
						NOTIFY_ERROR(mH_, "SetPageStoreStringMapValue: LoadStringMapPage failed, key:", mKey);
						mCompletionFunction->Call(mH_, stringmap::kSetStringMapValueResult_Error);
						return;
					}
					
					mMapPage->mMap = inMap;
					SetValue(mH_, mMapPage, mKey, mValue, mCompletionFunction);
				}
				
				//
				//
				HermitPtr mH_;
				StringMapPagePtr mMapPage;
				std::string mKey;
				std::string mValue;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			//
			void PerformWork(const HermitPtr& h_,
							 const stringmap::StringMapPtr& inStringMap,
							 const std::string& inKey,
							 const std::string& inValue,
							 const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction)
			{
				PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
				
				ThreadLockScope lock(stringMap.mLock);
				if (stringMap.mPages.empty())
				{
					StringMapPtr map(new StringMap());
					map->mEntries.insert(StringMap::Map::value_type(inKey, inValue));
					
					StringMapPagePtr page(new StringMapPage());
					page->mKey = " ";
					page->mMap = map;
					page->mDirty = true;
					
					stringMap.mPages.insert(PageStoreStringMap::PageMap::value_type(" ", page));
					
					inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Success);
					return;
				}
				
				auto pageIt = stringMap.mPages.lower_bound(inKey);
				if (pageIt != stringMap.mPages.begin())
				{
					pageIt--;
				}
				
				if (pageIt->second->mMap == nullptr)
				{
					auto completion = std::make_shared<LoadCallback>(h_,
																	 pageIt->second,
																	 inKey,
																	 inValue,
																	 inCompletionFunction);
					stringMap.LoadPage(h_, pageIt->second->mKey, completion);
					return;
				}
				
				SetValue(h_, pageIt->second, inKey, inValue, inCompletionFunction);
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
							 const std::string& inValue,
							 const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mValue(inValue),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				virtual void Call(const HermitPtr& h_, const InitPageStoreStringMapStatus& inStatus) override {
					if (inStatus == kInitPageStoreStringMapStatus_Canceled)
					{
						mCompletionFunction->Call(mH_, stringmap::kSetStringMapValueResult_Canceled);
						return;
					}
					if (inStatus != kInitPageStoreStringMapStatus_Success)
					{
						NOTIFY_ERROR(mH_, "SetPageStoreStringMapValue: Init failed.");
						mCompletionFunction->Call(mH_, stringmap::kSetStringMapValueResult_Error);
						return;
					}
					PerformWork(mH_, mStringMap, mKey, mValue, mCompletionFunction);
				}
				
				//
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				std::string mValue;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletionFunction;
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
					 const std::string& inValue,
					 const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mH_(h_),
				mStringMap(inStringMap),
				mKey(inKey),
				mValue(inValue),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void PerformTask(
										 const int32_t& inTaskID)
				{
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					
					auto completion = std::make_shared<InitCallback>(mH_, mStringMap, mKey, mValue, mCompletionFunction);
					stringMap.Init(mH_, completion);
				}
				
				//
				//
				HermitPtr mH_;
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				std::string mValue;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			//
			class CompletionProxy
			:
			public stringmap::SetStringMapValueCompletionFunction
			{
			public:
				//
				//
				CompletionProxy(const stringmap::StringMapPtr& inStringMap,
								const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction)
				:
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction)
				{
				}
				
				//
				//
				virtual void Call(const HermitPtr& h_, const stringmap::SetStringMapValueResult& inResult) override {
					mCompletionFunction->Call(h_, inResult);
					
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					stringMap.TaskComplete();
				}
				
				//
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
		} // private namespace
		
		//
		//
		void SetPageStoreStringMapValue(const HermitPtr& h_,
										const stringmap::StringMapPtr& inStringMap,
										const std::string& inKey,
										const std::string& inValue,
										const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction)
		{
			if (inStringMap == nullptr)
			{
				NOTIFY_ERROR(h_, "SetPageStoreStringMapValue: inStringMap == null.");
				inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Error);
				return;
			}
			if (inKey.empty())
			{
				NOTIFY_ERROR(h_, "SetPageStoreStringMapValue: null key.");
				inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Error);
				return;
			}
			if (inKey[0] < ' ')
			{
				NOTIFY_ERROR(h_, "SetPageStoreStringMapValue: invalid key.");
				inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Error);
				return;
			}
			
			stringmap::SetStringMapValueCompletionFunctionPtr completion(new CompletionProxy(inStringMap, inCompletionFunction));
			auto task = std::make_shared<Task>(h_, inStringMap, inKey, inValue, completion);
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			if (!stringMap.QueueTask(task)) {
				NOTIFY_ERROR(h_, "SetPageStoreStringMapValue: QueueTask failed.");
				inCompletionFunction->Call(h_, stringmap::kSetStringMapValueResult_Error);
			}
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
