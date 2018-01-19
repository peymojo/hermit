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

#ifndef GetFileTotalSize_h
#define GetFileTotalSize_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
    namespace file {
        
        //
        DEFINE_CALLBACK_3A(GetFileTotalSizeCallback,
                           HermitPtr,
                           bool,                         // success
                           std::uint64_t);               // totalSize
        
        //
        class GetFileTotalSizeCallbackClass : public GetFileTotalSizeCallback {
        public:
            //
            GetFileTotalSizeCallbackClass() : mSuccess(false), mTotalSize(0) {
            }
            
            //
            virtual bool Function(const HermitPtr& h_, const bool& success, const uint64_t& totalSize) override {
                mSuccess = success;
                if (success) {
                    mTotalSize = totalSize;
                }
                return true;
            }
            
            //
            bool mSuccess;
            uint64_t mTotalSize;
        };
        
        //
        void GetFileTotalSize(const HermitPtr& h_,
                              const FilePathPtr& filePath,
                              const GetFileTotalSizeCallbackRef& callback);
        
    } // namespace file
} // namespace hermit

#endif

