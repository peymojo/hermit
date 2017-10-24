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

#ifndef DataStorePageStore_h
#define DataStorePageStore_h

#include <map>
#include <string>
#include <vector>
#include "Hermit/DataStore/DataPath.h"
#include "Hermit/DataStore/DataStore.h"
#include "Hermit/Foundation/TaskQueue.h"
#include "Hermit/Foundation/ThreadLock.h"
#include "Hermit/PageStore/PageStore.h"

namespace hermit {
	namespace datastorepagestore {
		
		//
		enum class ReadPageTableResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(ReadPageTableCompletionFunction, HermitPtr, ReadPageTableResult);
		
		//
		class DataStorePageStore : public pagestore::PageStore, public TaskQueue,
		public std::enable_shared_from_this<DataStorePageStore> {
		public:
			//
			DataStorePageStore(const datastore::DataStorePtr& inDataStore, const datastore::DataPathPtr& inPath);
			
			//
			virtual void EnumeratePages(const HermitPtr& h_,
										const pagestore::EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
										const pagestore::EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual void ReadPage(const HermitPtr& h_,
								  const std::string& inPageName,
								  const pagestore::ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual void WritePage(const HermitPtr& h_,
								   const std::string& inPageName,
								   const DataBuffer& inPageData,
								   const pagestore::WritePageStorePageCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual void Lock(const HermitPtr& h_,
							  const pagestore::LockPageStoreCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual void Unlock(const HermitPtr& h_) override;
			
			//
			virtual void CommitChanges(const HermitPtr& h_,
									   const pagestore::CommitPageStoreChangesCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			virtual void Validate(const HermitPtr& h_,
								  const pagestore::ValidatePageStoreCompletionFunctionPtr& inCompletionFunction) override;
			
			//
			void ReadPageTable(const HermitPtr& h_, const ReadPageTableCompletionFunctionPtr& inCompletionFunction);
			
			//
			datastore::DataStorePtr mDataStore;
			datastore::DataPathPtr mPath;
			
			//
			typedef std::map<std::string, std::string> PageMap;
			PageMap mPageTable;
			typedef std::map<std::string, SharedBufferPtr> PageDataMap;
			PageDataMap mDirtyPages;
			bool mPageTableLoaded;
		};
		
	} // namespace datastorepagestore
} // namespace hermit

#endif
