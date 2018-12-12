//
//    Hermit
//    Copyright (C) 2017 Paul Young (aka peymojo)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#ifndef AES256EncryptedS3DataStore_h
#define AES256EncryptedS3DataStore_h

#include <string>
#include "S3DataStore.h"

namespace hermit {
    namespace s3datastore {
        
        //
        class AES256EncryptedS3DataStore : public S3DataStore {
        public:
            //
            AES256EncryptedS3DataStore(s3bucket::S3BucketPtr bucket,
                                       bool useReducedRedundancyStorage,
                                       const std::string& aesKey);
            
            //
            virtual void LoadData(const HermitPtr& h_,
                                  const datastore::DataPathPtr& path,
                                  const datastore::EncryptionSetting& encryptionSetting,
                                  const datastore::LoadDataStoreDataDataBlockPtr& dataBlock,
                                  const datastore::LoadDataStoreDataCompletionBlockPtr& completion) override;
            
            //
            virtual void WriteData(const HermitPtr& h_,
                                   const datastore::DataPathPtr& path,
                                   const SharedBufferPtr& data,
                                   const datastore::EncryptionSetting& encryptionSetting,
                                   const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) override;
            
            //
            std::string mAESKey;
        };
        typedef std::shared_ptr<AES256EncryptedS3DataStore> AES256EncryptedS3DataStorePtr;
        
    } // namespace s3datastore
} // namespace hermit

#endif

