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

#ifndef CountDirectoryContentsWithSize_h
#define CountDirectoryContentsWithSize_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"
#include "PreprocessFileFunction.h"

namespace hermit {
	namespace file {
		
		//
		//
		enum CountDirectoryContentsWithSizeStatus
		{
			kCountDirectoryContentsWithSizeStatus_Unknown,
			kCountDirectoryContentsWithSizeStatus_Success,
			kCountDirectoryContentsWithSizeStatus_Canceled,
			kCountDirectoryContentsWithSizeStatus_DirectoryNotFound,
			kCountDirectoryContentsWithSizeStatus_PermissionDenied,
			kCountDirectoryContentsWithSizeStatus_Error
		};
		
		//
		//
		DEFINE_CALLBACK_3A(CountDirectoryContentsWithSizeCallback,
						   CountDirectoryContentsWithSizeStatus,		// inStatus
						   uint64_t,									// inCount
						   uint64_t);									// inSize
		
		//
		//
		class CountDirectoryContentsWithSizeCallbackClass
		:
		public CountDirectoryContentsWithSizeCallback
		{
		public:
			//
			//
			CountDirectoryContentsWithSizeCallbackClass()
			:
			mStatus(kCountDirectoryContentsWithSizeStatus_Unknown),
			mCount(0),
			mSize(0)
			{
			}
			
			//
			//
			bool Function(
						  const CountDirectoryContentsWithSizeStatus& inStatus,
						  const uint64_t& inCount,
						  const uint64_t& inSize)
			{
				mStatus = inStatus;
				if (inStatus == kCountDirectoryContentsWithSizeStatus_Success)
				{
					mCount = inCount;
					mSize = inSize;
				}
				return true;
			}
			
			//
			//
			CountDirectoryContentsWithSizeStatus mStatus;
			uint64_t mCount;
			uint64_t mSize;
		};
		
		//
		//
		void CountDirectoryContentsWithSize(const HermitPtr& h_,
											const FilePathPtr& inDirectoryPath,
											const bool& inDescendSubdirectories,
											const bool& inIgnorePermissionsErrors,
											const PreprocessFileFunctionPtr& inPreprocessFunction,
											const CountDirectoryContentsWithSizeCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
