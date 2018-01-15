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

#include "S3DataStore.h"

namespace hermit {
	namespace s3datastore {
		
		//
		void S3DataStore::CreateLocationIfNeeded(const HermitPtr& h_,
                                                 const datastore::DataPathPtr& path,
                                                 const datastore::CreateDataStoreLocationIfNeededCompletionPtr& completion) {
			//	we don't have to create anything for S3, since "folders" are just path-like strings in an object key.
            completion->Call(h_, datastore::CreateDataStoreLocationIfNeededResult::kSuccess);
		}
		
	} // namespace s3datastore
} // namespace hermit
