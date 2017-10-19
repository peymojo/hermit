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

#ifndef ValuesPromise_h
#define ValuesPromise_h

#include <memory>
#include "Value.h"

namespace hermit {
	namespace value {

		// mechanism to avoid parsing data values until / if needed:
		class ValuesPromise {
		public:
			//
			virtual ~ValuesPromise() = default;
			//
			virtual bool GetValues(const hermit::HermitPtr& h_, hermit::value::ValuePtr& outValues) = 0;
		};
		typedef std::shared_ptr<ValuesPromise> ValuesPromisePtr;

	} // namespace value
} // namespace hermit

#endif /* ValuesPromise_h */
