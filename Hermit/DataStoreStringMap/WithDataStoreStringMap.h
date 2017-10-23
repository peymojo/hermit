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

#ifndef WithDataStoreStringMap_h
#define WithDataStoreStringMap_h

#include "Hermit/DataStore/DataPath.h"
#include "Hermit/DataStore/DataStore.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/StringMap/StringMap.h"

namespace hermit {
	namespace datastorestringmap {
		
		//
		stringmap::WithStringMapResult WithDataStoreStringMap(const HermitPtr& h_,
															  const datastore::DataStorePtr& inDataStore,
															  const datastore::DataPathPtr& inPath,
															  stringmap::StringMapPtr& outStringMap);
		
	} // namespace datastorestringmap
} // namespace hermit

#endif
