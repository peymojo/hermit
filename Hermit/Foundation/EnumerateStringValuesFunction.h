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

#ifndef EnumerateStringValuesFunction_h
#define EnumerateStringValuesFunction_h

#include <string>
#include <vector>
#include "Hermit/Foundation/Callback.h"

namespace hermit {
	
	//
	typedef std::vector<std::pair<std::string, std::string>> StringPairVector;
	
	// name, value
	DEFINE_CALLBACK_2A(EnumerateStringValuesCallback, std::string, std::string);
	
	//
	class EnumerateStringValuesCallbackClass : public EnumerateStringValuesCallback {
	public:
		//
		bool Function(const std::string& name, const std::string& value) {
			mValues.push_back(std::make_pair(name, value));
			return true;
		}
		
		//
		StringPairVector mValues;
	};
	
	//
	DEFINE_CALLBACK_1A(EnumerateStringValuesFunction, EnumerateStringValuesCallbackRef);
	
	//
	class EnumerateStringValuesFunctionClass : public EnumerateStringValuesFunction {
	public:
		//
		EnumerateStringValuesFunctionClass(const StringPairVector& inValues) : mValues(inValues) {
		}
		
		//
		bool Function(const EnumerateStringValuesCallbackRef& inCallback) {
			auto end = mValues.end();
			for (auto it = mValues.begin(); it != end; ++it) {
				if (!inCallback.Call((*it).first, (*it).second)) {
					return false;
				}
			}
			return true;
		}
		
		//
		const StringPairVector& mValues;
	};
	
} // namespace hermit

#endif
