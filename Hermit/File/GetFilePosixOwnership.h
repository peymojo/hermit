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

#ifndef GetFilePosixOwnership_h
#define GetFilePosixOwnership_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_3A(
						   GetFilePosixOwnershipCallback,
						   bool,							// inSuccess
						   std::string,					// inUserOwner
						   std::string);					// inGroupOwner
		
		//
		//
		template <class StringT>
		class GetFilePosixOwnershipCallbackClassT
		:
		public GetFilePosixOwnershipCallback
		{
		public:
			//
			//
			GetFilePosixOwnershipCallbackClassT()
			:
			mSuccess(false)
			{
			}
			
			//
			//
			bool Function(
						  const bool& inSuccess,
						  const std::string& inUserOwner,
						  const std::string& inGroupOwner)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mUserOwner = inUserOwner;
					mGroupOwner = inGroupOwner;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			StringT mUserOwner;
			StringT mGroupOwner;
		};
		
		//
		//
		void GetFilePosixOwnership(const HermitPtr& h_,
								   const FilePathPtr& inFilePath,
								   const GetFilePosixOwnershipCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
