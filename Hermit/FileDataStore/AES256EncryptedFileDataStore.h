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

#ifndef AES256EncryptedFileDataStore_h
#define AES256EncryptedFileDataStore_h

#include <string>
#include "FileDataStore.h"

namespace hermit {
	namespace filedatastore {

		//
		class AES256EncryptedFileDataStore : public FileDataStore {
		public:
			//
			AES256EncryptedFileDataStore(const std::string& aesKey);
			
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
            void DoWriteData(const HermitPtr& h_,
                             const datastore::DataPathPtr& path,
                             const SharedBufferPtr& data,
                             const datastore::EncryptionSetting& encryptionSetting,
                             const datastore::WriteDataStoreDataCompletionFunctionPtr& completion);

            //
			std::string mAESKey;
		};
		typedef std::shared_ptr<AES256EncryptedFileDataStore> AES256EncryptedFileDataStorePtr;

	} // namespace filedatastore
} // namespace hermit

#endif 
