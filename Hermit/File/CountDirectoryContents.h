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

#ifndef CountDirectoryContents_h
#define CountDirectoryContents_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "PreprocessFileFunction.h"

namespace hermit {
	namespace file {
		
		//
		//
		enum CountDirectoryContentsStatus
		{
			kCountDirectoryContentsStatus_Unknown,
			kCountDirectoryContentsStatus_Success,
			kCountDirectoryContentsStatus_Canceled,
			kCountDirectoryContentsStatus_DirectoryNotFound,
			kCountDirectoryContentsStatus_PermissionDenied,
			kCountDirectoryContentsStatus_Error
		};
		
		//
		//
		DEFINE_CALLBACK_2A(
						   CountDirectoryContentsCallback,
						   CountDirectoryContentsStatus,		// inStatus
						   uint64_t);							// inCount
		
		//
		//
		class CountDirectoryContentsCallbackClass
		:
		public CountDirectoryContentsCallback
		{
		public:
			//
			//
			CountDirectoryContentsCallbackClass()
			:
			mStatus(kCountDirectoryContentsStatus_Unknown),
			mFileCount(0)
			{
			}
			
			//
			//
			bool Function(
						  const CountDirectoryContentsStatus& inStatus,
						  const uint64_t& inFileCount)
			{
				mStatus = inStatus;
				if (inStatus == kCountDirectoryContentsStatus_Success)
				{
					mFileCount = inFileCount;
				}
				return true;
			}
			
			//
			//
			CountDirectoryContentsStatus mStatus;
			uint64_t mFileCount;
		};
		
		//
		//
		void CountDirectoryContents(const HermitPtr& h_,
									const FilePathPtr& inDirectoryPath,
									const bool& inDescendSubdirectories,
									const bool& inIgnorePermissionsErrors,
									const PreprocessFileFunctionPtr& inPreprocessFunction,
									const CountDirectoryContentsCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
