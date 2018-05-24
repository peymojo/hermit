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
#include "Hermit/Value/Value.h"

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
		DEFINE_ASYNC_FUNCTION_3A(GetStringMapValueCompletion,
								 HermitPtr,
								 GetStringMapValueResult,					// result
								 std::string);								// value
		

		//
		enum class SetStringMapValueResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_2A(SetStringMapValueCompletion, HermitPtr, SetStringMapValueResult);
		
		//
		struct StringMap {
			//
			virtual void GetValue(const HermitPtr& h_,
								  const std::string& key,
								  const GetStringMapValueCompletionPtr& completion);

			//
			virtual void SetValue(const HermitPtr& h_,
								  const std::string& key,
								  const std::string& value,
								  const SetStringMapValueCompletionPtr& completion);
			
			//
			virtual bool ExportValues(const HermitPtr& h_, value::ValuePtr& outValues);
			
		protected:
			//
			virtual ~StringMap() = default;
		};
		typedef std::shared_ptr<StringMap> StringMapPtr;

	} // namespace stringmap
} // namespace hermit

#endif
