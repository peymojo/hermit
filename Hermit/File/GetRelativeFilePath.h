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

#ifndef GetRelativeFilePath_h
#define GetRelativeFilePath_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		DEFINE_CALLBACK_2A(GetRelativeFilePathCallback,
						   bool,							// inSuccess
						   FilePathPtr);					// inRelativePath
		
		//
		template <class ManagedFilePathPtrT>
		class GetRelativeFilePathCallbackClassT : public GetRelativeFilePathCallback {
		public:
			//
			GetRelativeFilePathCallbackClassT() : mSuccess(false) {
			}
			
			//
			bool Function(const bool& inSuccess, const FilePathPtr& inRelativeFilePath) {
				mSuccess = inSuccess;
				if (inSuccess) {
					mFilePath = ManagedFilePathPtrT(inRelativeFilePath);
				}
				return true;
			}
			
			//
			bool mSuccess;
			ManagedFilePathPtrT mFilePath;
		};
		
		//
		void GetRelativeFilePath(const HermitPtr& h_,
								 const FilePathPtr& inFromPath,
								 const FilePathPtr& inToPath,
								 const GetRelativeFilePathCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
