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
#include "WithS3DataStore.h"

namespace hermit {
	namespace s3datastore {
		
		//
		bool WithS3DataStore(const hermit::HermitPtr& h_,
							 const s3bucket::S3BucketPtr& s3Bucket,
							 const bool& useReducedRedundancyStorage,
							 datastore::DataStorePtr& outDataStore) {
			outDataStore = std::make_shared<S3DataStore>(s3Bucket, useReducedRedundancyStorage);
			return true;
		}
		
	} // namespace s3datastore
} // namespace hermit

