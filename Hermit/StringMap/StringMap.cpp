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
#include "StringMap.h"

namespace hermit {
	namespace stringmap {

		//
		void StringMap::GetValue(const HermitPtr& h_,
								 const std::string& key,
								 const GetStringMapValueCompletionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
			completion->Call(h_, GetStringMapValueResult::kError, "");
		}
		
		//
		void StringMap::SetValue(const HermitPtr& h_,
								 const std::string& inKey,
								 const std::string& inValue,
								 const SetStringMapValueCompletionPtr& completion) {
			NOTIFY_ERROR(h_, "unimplemented");
			completion->Call(h_, SetStringMapValueResult::kError);
		}

		//
		bool StringMap::ExportValues(const HermitPtr& h_, value::ValuePtr& outValues) {
			NOTIFY_ERROR(h_, "unimplemented");
			return false;
		}

	} // namespace stringmap
} // namespace hermit
