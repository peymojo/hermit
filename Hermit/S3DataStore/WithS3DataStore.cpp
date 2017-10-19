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
//
void WithS3DataStore(
	const s3bucket::S3BucketPtr& inS3Bucket,
	const bool& inUseReducedRedundancyStorage,
	const datastore::WithDataStoreCallbackRef& inCallback)
{
	auto dataStore = std::make_shared<S3DataStore>(inS3Bucket, inUseReducedRedundancyStorage);
	inCallback.Call(datastore::kWithDataStoreCallbackStatus_Success, dataStore);
}

} // namespace s3datastore
} // namespace hermit
