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

#include "Hermit/Foundation/Notification.h"
#include "S3DataPath.h"
#include "S3DataStore.h"

namespace hermit {
	namespace s3datastore {
        namespace S3DataStore_LoadData_Impl {
            
            //
            class CompletionBlock : public s3::S3CompletionBlock {
            public:
                //
                CompletionBlock(const datastore::DataPathPtr& inPath,
                                const s3::GetS3ObjectResponsePtr& inResponse,
                                const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
                                const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) :
                mPath(inPath),
                mResponse(inResponse),
                mDataBlock(inDataBlock),
                mCompletion(inCompletion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kCanceled);
                        return;
                    }
                    if (result == s3::S3Result::k403AccessDenied) {
                        mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kPermissionDenied);
                        return;
                    }
                    if (result == s3::S3Result::k404EntityNotFound) {
                        mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kItemNotFound);
                        return;
                    }
                    if (result == s3::S3Result::k404NoSuchBucket) {
                        mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kDataStoreMissing);
                        return;
                    }
                    if (result != s3::S3Result::kSuccess) {
                        NOTIFY_ERROR(h_, "LoadS3DataStoreData: GetObjectFromS3Bucket failed for:", mPath);
                        mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
                        return;
                    }
                    mDataBlock->Call(h_, DataBuffer(mResponse->mData.data(), mResponse->mData.size()));
                    mCompletion->Call(h_, datastore::LoadDataStoreDataResult::kSuccess);
                }
                
                //
                datastore::DataPathPtr mPath;
                s3::GetS3ObjectResponsePtr mResponse;
                datastore::LoadDataStoreDataDataBlockPtr mDataBlock;
                datastore::LoadDataStoreDataCompletionBlockPtr mCompletion;
            };
            
        } // namespace S3DataStore_LoadData_Impl
        using namespace S3DataStore_LoadData_Impl;
		
		//
        void S3DataStore::LoadData(const HermitPtr& h_,
                                   const datastore::DataPathPtr& inPath,
                                   const datastore::EncryptionSetting& inEncryptionSetting,
                                   const datastore::LoadDataStoreDataDataBlockPtr& inDataBlock,
                                   const datastore::LoadDataStoreDataCompletionBlockPtr& inCompletion) {
			if (CHECK_FOR_ABORT(h_)) {
				inCompletion->Call(h_, datastore::LoadDataStoreDataResult::kCanceled);
				return;
			}
			
			S3DataPath& dataPath = static_cast<S3DataPath&>(*inPath);
			if (mBucket == nullptr) {
				NOTIFY_ERROR(h_, "LoadS3DataStoreData: dataStore.mBucket.P() == null for:", dataPath.mPath);
				inCompletion->Call(h_, datastore::LoadDataStoreDataResult::kError);
				return;
			}
			
			auto responseBlock = std::make_shared<s3::GetS3ObjectResponse>();
			auto completion = std::make_shared<CompletionBlock>(inPath, responseBlock, inDataBlock, inCompletion);
			mBucket->GetObject(h_, dataPath.mPath, responseBlock, completion);
		}
		
	} // namespace s3datastore
} // namespace hermit
