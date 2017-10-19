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

#ifndef GetAliasTarget_h
#define GetAliasTarget_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		enum GetAliasTargetStatus
		{
			kGetAliasTargetStatus_Unknown,
			kGetAliasTargetStatus_Success,
			kGetAliasTargetStatus_TargetNotFound,
			kGetAliasTargetStatus_Error
		};
		
		//
		//
		DEFINE_CALLBACK_3A(GetAliasTargetCallback,
						   GetAliasTargetStatus,		// inStatus
						   FilePathPtr,				// inTargetFilePath
						   bool);						// inIsRelativePath
		
		//
		//
		template <class ManagedFilePathPtrT>
		class GetAliasTargetCallbackClassT
		:
		public GetAliasTargetCallback
		{
		public:
			//
			//
			GetAliasTargetCallbackClassT()
			:
			mStatus(kGetAliasTargetStatus_Unknown),
			mIsRelativePath(false)
			{
			}
			
			//
			//
			bool Function(const GetAliasTargetStatus& inStatus,
						  const FilePathPtr& inTargetFilePath,
						  const bool& inIsRelativePath)
			{
				mStatus = inStatus;
				if (inStatus == kGetAliasTargetStatus_Success)
				{
					mFilePath = ManagedFilePathPtrT(inTargetFilePath);
					mIsRelativePath = inIsRelativePath;
				}
				return true;
			}
			
			//
			//
			GetAliasTargetStatus mStatus;
			ManagedFilePathPtrT mFilePath;
			bool mIsRelativePath;
		};
		
		//
		//
		void GetAliasTarget(const HermitPtr& h_,
							const FilePathPtr& inFilePath,
							const GetAliasTargetCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif

