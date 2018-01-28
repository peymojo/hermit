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
		namespace GetPageStoreStringMapValue_Impl {
			
			//
			void GetValue(const hermit::HermitPtr& h_,
						  const StringMapPtr& inMap,
						  const std::string& inKey,
						  const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
				auto mapIt = inMap->mEntries.find(inKey);
				if (mapIt == inMap->mEntries.end()) {
					inCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kEntryNotFound, "");
					return;
				}
				inCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kEntryFound, mapIt->second);
			}
			
			//
			class LoadCallback : public LoadStringMapPageCompletionFunction {
			public:
				//
				LoadCallback(const StringMapPagePtr& inMapPage,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mMapPage(inMapPage),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const LoadStringMapPageResult& inResult,
								  const StringMapPtr& inMap) override {
					if (inResult == LoadStringMapPageResult::kCanceled) {
						mCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kCanceled, "");
						return;
					}
					if (inResult == LoadStringMapPageResult::kPageNotFound) {
						NOTIFY_ERROR(h_, "GetPageStoreStringMapValue: kPageNotFound, key:", mKey);
						mCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kError, "");
						return;
					}
					if (inResult != LoadStringMapPageResult::kSuccess) {
						NOTIFY_ERROR(h_, "GetPageStoreStringMapValue: LoadStringMapPage failed, key:", mKey);
						mCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kError, "");
						return;
					}
					
					mMapPage->mMap = inMap;
					GetValue(h_, inMap, mKey, mCompletionFunction);
				}
				
				//
				StringMapPagePtr mMapPage;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			void PerformWork(const HermitPtr& h_,
							 const stringmap::StringMapPtr& inMap,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
				PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inMap);
				
				ThreadLockScope lock(stringMap.mLock);
				
				if (stringMap.mPages.empty()) {
					inCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kEntryNotFound, "");
					return;
				}
				
				auto pageIt = stringMap.mPages.lower_bound(inKey);
				if (pageIt != stringMap.mPages.begin()) {
					pageIt--;
				}
				
				if (pageIt->second->mMap == nullptr) {
					auto completion = std::make_shared<LoadCallback>(pageIt->second, inKey, inCompletionFunction);
					stringMap.LoadPage(h_, pageIt->second->mKey, completion);
					return;
				}
				
				GetValue(h_, pageIt->second->mMap, inKey, inCompletionFunction);
			}
			
			//
			class InitCallback : public InitPageStoreStringMapCompletionFunction {
			public:
				//
				InitCallback(const stringmap::StringMapPtr& inStringMap,
							 const std::string& inKey,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const InitPageStoreStringMapStatus& inStatus) override {
					if (inStatus == kInitPageStoreStringMapStatus_Canceled) {
						mCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kCanceled, "");
						return;
					}
					if (inStatus != kInitPageStoreStringMapStatus_Success) {
						NOTIFY_ERROR(h_, "GetPageStoreStringMapValue: Init failed.");
						mCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kError, "");
						return;
					}
					PerformWork(h_, mStringMap, mKey, mCompletionFunction);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class Task : public AsyncTask {
			public:
				//
				Task(const stringmap::StringMapPtr& inStringMap,
					 const std::string& inKey,
					 const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mStringMap(inStringMap),
				mKey(inKey),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					auto completion = std::make_shared<InitCallback>(mStringMap, mKey, mCompletionFunction);
					stringMap.Init(h_, completion);
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
			//
			class CompletionProxy : public stringmap::GetStringMapValueCompletionFunction {
			public:
				//
				CompletionProxy(const stringmap::StringMapPtr& inStringMap,
								const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) :
				mStringMap(inStringMap),
				mCompletionFunction(inCompletionFunction) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const stringmap::GetStringMapValueResult& inResult,
								  const std::string& inValue) override {
					mCompletionFunction->Call(h_, inResult, inValue);
					PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*mStringMap);
					stringMap.TaskComplete();
				}
				
				//
				stringmap::StringMapPtr mStringMap;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletionFunction;
			};
			
		} // namespace GetPageStoreStringMapValue_Impl
		using namespace GetPageStoreStringMapValue_Impl;
		
		//
		void GetPageStoreStringMapValue(const HermitPtr& h_,
										const stringmap::StringMapPtr& inStringMap,
										const std::string& inKey,
										const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) {
			auto completion = std::make_shared<CompletionProxy>(inStringMap, inCompletionFunction);
			auto task = std::make_shared<Task>(inStringMap, inKey, completion);
			PageStoreStringMap& stringMap = static_cast<PageStoreStringMap&>(*inStringMap);
			if (!stringMap.QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "stringMap.QueueTask failed");
				inCompletionFunction->Call(h_, stringmap::GetStringMapValueResult::kError, "");
			}
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
