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

#ifndef WithAES256EncryptedS3DataStore_h
#define WithAES256EncryptedS3DataStore_h

#include "Hermit/DataStore/DataStore.h"
#include "Hermit/S3Bucket/S3Bucket.h"

namespace hermit {
	namespace s3datastore {
		
		//
		bool WithAES256EncryptedS3DataStore(const hermit::HermitPtr& h_,
											const s3bucket::S3BucketPtr& s3Bucket,
											const bool& useReducedRedundancyStorage,
											const std::string& aesKey,
											datastore::DataStorePtr& outDataStore);
		
	} // namespace s3datastore
} // namespace hermit

#endif

