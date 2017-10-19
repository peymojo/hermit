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

#include <string>
#include "S3DataPath.h"
#include "S3DataStore.h"
#include "DeleteS3DataStoreItem.h"

namespace hermit {
	namespace s3datastore {
		
		//
		bool DeleteS3DataStoreItem(const HermitPtr& h_,
								   const datastore::DataStorePtr& inDataStore,
								   const datastore::DataPathPtr& inPath) {
			S3DataStore& dataStore = static_cast<S3DataStore&>(*inDataStore);
			S3DataPath& dataPath = static_cast<S3DataPath&>(*inPath);
			
			auto result = dataStore.mBucket->DeleteObject(h_, dataPath.mPath);
			return (result == s3::S3Result::kSuccess);
		}
		
	} // namespace s3datastore
} // namespace hermit
