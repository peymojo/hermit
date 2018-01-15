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

#include "S3DataPath.h"
#include "S3DataStore.h"

namespace hermit {
	namespace s3datastore {
        namespace S3DataStore_DeleteItem_Impl {
            
            //
            class DeleteCompletion : public s3::S3CompletionBlock {
            public:
                //
                DeleteCompletion(const datastore::DeleteDataStoreItemCompletionPtr& completion) :
                mCompletion(completion) {
                }
                
                //
                virtual void Call(const HermitPtr& h_, const s3::S3Result& result) override {
                    if (result == s3::S3Result::kCanceled) {
                        mCompletion->Call(h_, datastore::DeleteDataStoreItemResult::kCanceled);
                        return;
                    }
                    if (result != s3::S3Result::kSuccess) {
                        mCompletion->Call(h_, datastore::DeleteDataStoreItemResult::kError);
                        return;
                    }
                    mCompletion->Call(h_, datastore::DeleteDataStoreItemResult::kSuccess);
                }
                
                //
                datastore::DeleteDataStoreItemCompletionPtr mCompletion;
            };
            
        } // namespace S3DataStore_DeleteItem_Impl
        using namespace S3DataStore_DeleteItem_Impl;
		
		//
        void S3DataStore::DeleteItem(const HermitPtr& h_,
                                     const datastore::DataPathPtr& path,
                                     const datastore::DeleteDataStoreItemCompletionPtr& completion) {
			S3DataPath& dataPath = static_cast<S3DataPath&>(*path);
            auto deleteCompletion = std::make_shared<DeleteCompletion>(completion);
			mBucket->DeleteObject(h_, dataPath.mPath, deleteCompletion);
		}
		
	} // namespace s3datastore
} // namespace hermit
