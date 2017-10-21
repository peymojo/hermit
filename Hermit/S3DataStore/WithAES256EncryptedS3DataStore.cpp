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

#include "AES256EncryptedS3DataStore.h"
#include "WithAES256EncryptedS3DataStore.h"

namespace hermit {
	namespace s3datastore {
		
		//
		bool WithAES256EncryptedS3DataStore(const s3bucket::S3BucketPtr& s3Bucket,
											const bool& useReducedRedundancyStorage,
											const std::string& aesKey,
											datastore::DataStorePtr& outDataStore) {
			outDataStore = std::make_shared<AES256EncryptedS3DataStore>(s3Bucket,
																		useReducedRedundancyStorage,
																		aesKey);
			return true;
		}
		
	} // namespace s3datastore
} // namespace hermit

