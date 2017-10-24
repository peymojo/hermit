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

#ifndef PageStore_h
#define PageStore_h

#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace pagestore {

		//
		enum EnumeratePageStorePagesResult {
			kEnumeratePageStorePagesResult_Unknown,
			kEnumeratePageStorePagesResult_Success,
			kEnumeratePageStorePagesResult_Canceled,
			kEnumeratePageStorePagesResult_Error
		};
		
		// inPageName
		DEFINE_ASYNC_ENUMERATION_1A(EnumeratePageStorePagesEnumerationFunction, std::string);
		
		//
		DEFINE_ASYNC_FUNCTION_2A(EnumeratePageStorePagesCompletionFunction,
								 HermitPtr,
								 EnumeratePageStorePagesResult);			// inResult

		
		//
		enum class ReadPageStorePageResult {
			kUnknown,
			kSuccess,
			kPageNotFound,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(ReadPageStorePageCompletionFunction,
								 HermitPtr,
								 ReadPageStorePageResult,					// inResult
								 DataBuffer);								// inPageData
		
		
		//
		enum WritePageStorePageResult {
			kWritePageStorePageResult_Unknown,
			kWritePageStorePageResult_Success,
			kWritePageStorePageResult_Canceled,
			kWritePageStorePageResult_Error
		};
		
		//
		DEFINE_ASYNC_FUNCTION_1A(WritePageStorePageCompletionFunction,
								 WritePageStorePageResult);					// inResult
		
		
		//
		enum LockPageStoreStatus {
			kLockPageStoreStatus_Unknown,
			kLockPageStoreStatus_Success,
			kLockPageStoreStatus_Canceled,
			kLockPageStoreStatus_Error
		};
		
		//
		DEFINE_ASYNC_FUNCTION_1A(LockPageStoreCompletionFunction, LockPageStoreStatus);
		
		
		//
		enum class CommitPageStoreChangesResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(CommitPageStoreChangesCompletionFunction, HermitPtr, CommitPageStoreChangesResult);
		

		//
		enum class ValidatePageStoreResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(ValidatePageStoreCompletionFunction,
								 HermitPtr,
								 ValidatePageStoreResult);				// inStatus
		
		
		//
		struct PageStore {
			//
			virtual void EnumeratePages(const HermitPtr& h_,
										const EnumeratePageStorePagesEnumerationFunctionPtr& inEnumerationFunction,
										const EnumeratePageStorePagesCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void ReadPage(const HermitPtr& h_,
								  const std::string& inPageName,
								  const ReadPageStorePageCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void WritePage(const HermitPtr& h_,
								   const std::string& inPageName,
								   const DataBuffer& inPageData,
								   const WritePageStorePageCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void Lock(const HermitPtr& h_,
							  const LockPageStoreCompletionFunctionPtr& inCompletionFunction);
			
			//
			virtual void Unlock(const HermitPtr& h_);

			//
			virtual void CommitChanges(const HermitPtr& h_,
									   const CommitPageStoreChangesCompletionFunctionPtr& inCompletionFunction);
			
			//
			virtual void Validate(const HermitPtr& h_,
								  const ValidatePageStoreCompletionFunctionPtr& inCompletionFunction);
			
		protected:
			//
			virtual ~PageStore() = default;
		};
		typedef std::shared_ptr<PageStore> PageStorePtr;

	} // namespace pagestore
} // namespace hermit

#endif
