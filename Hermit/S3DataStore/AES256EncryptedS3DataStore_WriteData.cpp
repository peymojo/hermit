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

#include "Hermit/Encoding/AES256EncryptCBC.h"
#include "Hermit/Encoding/CreateInputVector.h"
#include "Hermit/Foundation/Notification.h"
#include "AES256EncryptedS3DataStore.h"

namespace hermit {
    namespace s3datastore {
		
        //
        void AES256EncryptedS3DataStore::WriteData(const HermitPtr& h_,
                                                   const datastore::DataPathPtr& path,
                                                   const SharedBufferPtr& data,
                                                   const datastore::EncryptionSetting& encryptionSetting,
                                                   const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
			if (CHECK_FOR_ABORT(h_)) {
				completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
				return;
			}
			
			std::string encryptedS3Data;
			if (encryptionSetting == datastore::EncryptionSetting::kUnencrypted) {
				encryptedS3Data.assign(data->Data(), data->Size());
			}
			else {
				std::string inputVector;
				if (!encoding::CreateInputVector(h_, 16, inputVector)) {
					NOTIFY_ERROR(h_, "CreateInputVector failed.");
					completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
					return;
				}
				encoding::AES256EncryptCBCCallbackClass dataCallback;
				encoding::AES256EncryptCBC(h_,
										   DataBuffer(data->Data(), data->Size()),
										   mAESKey,
										   inputVector,
										   dataCallback);
				
				if (dataCallback.mStatus == encoding::kAES256EncryptCBC_Canceled) {
					completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
					return;
				}
				if (dataCallback.mStatus != encoding::kAES256EncryptCBC_Success) {
					NOTIFY_ERROR(h_, "AES256EncryptCBC failed.");
					completion->Call(h_, datastore::WriteDataStoreDataResult::kError);
					return;
				}
				
				encryptedS3Data = inputVector;
				encryptedS3Data += dataCallback.mValue;
			}
			
			SharedBufferPtr buffer = std::make_shared<SharedBuffer>(encryptedS3Data);
			S3DataStore::WriteData(h_,
								   path,
								   buffer,
								   encryptionSetting,
								   completion);
        }
		
    } // namespace s3datastore
} // namespace hermit

