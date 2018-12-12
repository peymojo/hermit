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

#include <string>
#include "Hermit/Foundation/Notification.h"
#include "S3DataPath.h"
#include "S3DataStore.h"

namespace hermit {
    namespace s3datastore {
        namespace S3DataStore_WriteData_Impl {
            
            //
            class PutCompletion : public s3::PutS3ObjectCompletion {
            public:
                //
                PutCompletion(const std::string& dataPath,
                              const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) :
                mDataPath(dataPath),
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result, const std::string& version) override {
                    datastore::WriteDataStoreDataResult dataStoreResult = datastore::WriteDataStoreDataResult::kUnknown;
                    if (result == s3::S3Result::kSuccess) {
                        dataStoreResult = datastore::WriteDataStoreDataResult::kSuccess;
                    }
                    else if (result == s3::S3Result::kCanceled) {
                        dataStoreResult = datastore::WriteDataStoreDataResult::kCanceled;
                    }
                    else if (result == s3::S3Result::k403AccessDenied) {
                        NOTIFY_ERROR(h_, "S3Result::k403AccessDenied for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kAccessDenied;
                    }
                    else if ((result == s3::S3Result::kS3InternalError) ||
                             (result == s3::S3Result::k500InternalServerError) ||
                             (result == s3::S3Result::k503ServiceUnavailable)) {
                        NOTIFY_ERROR(h_, "Server error for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kError;
                    }
                    else if (result == s3::S3Result::kHostNotFound) {
                        NOTIFY_ERROR(h_, "S3Result::kHostNotFound for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kError;
                    }
                    else if (result == s3::S3Result::kNetworkConnectionLost) {
                        NOTIFY_ERROR(h_, "S3Result::kNetworkConnectionLost for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kError;
                    }
                    else if (result == s3::S3Result::kNoNetworkConnection) {
                        NOTIFY_ERROR(h_, "S3Result::kNoNetworkConnection for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kError;
                    }
                    else if (result == s3::S3Result::kTimedOut) {
                        NOTIFY_ERROR(h_, "S3Result::kTimedOut for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kTimedOut;
                    }
                    else if (result == s3::S3Result::kError) {
                        NOTIFY_ERROR(h_, "S3Result::kError for:", mDataPath);
                        dataStoreResult = datastore::WriteDataStoreDataResult::kError;
                    }
                    else {
                        NOTIFY_ERROR(h_, "Unexpected result for:", mDataPath);
                    }
                    mCompletion->Call(h_, dataStoreResult);
                }
                
                //
                std::string mDataPath;
                datastore::WriteDataStoreDataCompletionFunctionPtr mCompletion;
            };
            
        } // namespace S3DataStore_WriteData_Impl
        using namespace S3DataStore_WriteData_Impl;
        
        //
        void S3DataStore::WriteData(const HermitPtr& h_,
                                    const datastore::DataPathPtr& path,
                                    const SharedBufferPtr& data,
                                    const datastore::EncryptionSetting& encryptionSetting,
                                    const datastore::WriteDataStoreDataCompletionFunctionPtr& completion) {
			if (CHECK_FOR_ABORT(h_)) {
				completion->Call(h_, datastore::WriteDataStoreDataResult::kCanceled);
				return;
			}
			
			S3DataPath& dataPath = static_cast<S3DataPath&>(*path);
			
			auto putCompletion = std::make_shared<PutCompletion>(dataPath.mPath, completion);
			mBucket->PutObject(h_,
							   dataPath.mPath,
							   data,
							   mUseReducedRedundancyStorage,
							   putCompletion);
        }
        
    } // namespace s3datastore
} // namespace hermit

