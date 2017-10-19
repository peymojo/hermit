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

#ifndef EnumerateDataValuesFunction_h
#define EnumerateDataValuesFunction_h

#include "Hermit/Foundation/Hermit.h"
#include "DataType.h"

namespace hermit {
	namespace value {
		
		//
		class EnumerateDataValuesCallback {
		public:
			//
			virtual bool Function(const HermitPtr& h_,
								  bool inSuccess,
								  const std::string& inName,
								  const DataType& inType,
								  const void* inValue) = 0;

			//
			bool Call(const HermitPtr& h_,
					  bool inSuccess,
					  const std::string& inName,
					  const DataType& inType,
					  const void* inValue) {
				return Function(h_, inSuccess, inName, inType, inValue);
			}
		};
		
		//
		class EnumerateDataValuesFunction {
		public:
			//
			virtual bool Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const = 0;
			
			//
			bool Call(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const {
				return Function(h_, inCallback);
			}
		};
		typedef std::shared_ptr<EnumerateDataValuesFunction> EnumerateDataValuesFunctionPtr;

	} // namespace value
} // namespace hermit

#endif
