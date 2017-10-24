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

#ifndef StringMap_h
#define StringMap_h

#include <memory>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace stringmap {

		//
		enum class WithStringMapResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kStringMapTooOld,
			kError
		};

		//
		enum class GetStringMapValueResult {
			kUnknown,
			kEntryFound,
			kEntryNotFound,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_3A(GetStringMapValueCompletionFunction,
								 HermitPtr,
								 GetStringMapValueResult,					// inStatus
								 std::string);								// inValue
		

		//
		enum class SetStringMapValueResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(SetStringMapValueCompletionFunction,
								 HermitPtr,
								 SetStringMapValueResult);					// inResult
		
		
		//
		enum class LockStringMapResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(LockStringMapCompletionFunction,
								 HermitPtr,
								 LockStringMapResult);
		

		//
		enum class CommitStringMapChangesResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(CommitStringMapChangesCompletionFunction,
								 HermitPtr,
								 CommitStringMapChangesResult);				// inStatus
		

		//
		enum class ValidateStringMapResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(ValidateStringMapCompletionFunction,
								 HermitPtr,
								 ValidateStringMapResult);					// inResult
		
		//
		struct StringMap {
			//
			virtual void GetValue(const HermitPtr& h_,
								  const std::string& inKey,
								  const GetStringMapValueCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void SetValue(const HermitPtr& h_,
								  const std::string& inKey,
								  const std::string& inValue,
								  const SetStringMapValueCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void Lock(const HermitPtr& h_,
							  const LockStringMapCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void Unlock(const HermitPtr& h_);

			//
			virtual void CommitChanges(const HermitPtr& h_,
									   const CommitStringMapChangesCompletionFunctionPtr& inCompletionFunction);

			//
			virtual void Validate(const HermitPtr& h_,
								  const ValidateStringMapCompletionFunctionPtr& inCompletionFunction);
			
		protected:
			//
			virtual ~StringMap() = default;
		};
		typedef std::shared_ptr<StringMap> StringMapPtr;

	} // namespace stringmap
} // namespace hermit

#endif
