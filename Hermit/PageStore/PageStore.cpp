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
#include "PageStore.h"

namespace hermit {
	namespace pagestore {
		
		//
		void PageStore::EnumeratePages(const HermitPtr& h_,
									   const EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
									   const EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::ReadPage(const HermitPtr& h_,
								 const std::string& inPageName,
								 const ReadPageStorePageCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::WritePage(const HermitPtr& h_,
								  const std::string& inPageName,
								  const DataBuffer& inPageData,
								  const WritePageStorePageCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::Lock(const HermitPtr& h_,
							 const LockPageStoreCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::Unlock(const HermitPtr& h_) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::CommitChanges(const HermitPtr& h_,
									  const CommitPageStoreChangesCompletionFunctionPtr& inCompletionFunction) {
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
		//
		void PageStore::Validate(const HermitPtr& h_,
								 const ValidatePageStoreCompletionFunctionPtr& inCompletionFunction){
			NOTIFY_ERROR(h_, "unimplemented");
		}
		
	} // namespace pagestore
} // namespace hermit
