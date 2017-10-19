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

#ifndef PathIsHardLink_h
#define PathIsHardLink_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_4A(
						   PathIsHardLinkCallback,
						   bool,						// inSuccess
						   bool,						// inIsHardLink
						   uint32_t,					// inFileSystemNumber
						   uint32_t);					// inFileNumber
		
		//
		//
		class PathIsHardLinkCallbackClass
		:
		public PathIsHardLinkCallback
		{
		public:
			//
			//
			PathIsHardLinkCallbackClass()
			:
			mSuccess(false),
			mIsHardLink(false),
			mFileSystemNumber(0),
			mFileNumber(0)
			{
			}
			
			//
			//
			bool Function(
						  const bool& inSuccess,
						  const bool& inIsHardLink,
						  const uint32_t& inFileSystemNumber,
						  const uint32_t& inFileNumber)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mIsHardLink = inIsHardLink;
					mFileSystemNumber = inFileSystemNumber;
					mFileNumber = inFileNumber;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			bool mIsHardLink;
			uint32_t mFileSystemNumber;
			uint32_t mFileNumber;
		};
		
		//
		//
		void PathIsHardLink(const HermitPtr& h_,
							const FilePathPtr& inPath,
							const PathIsHardLinkCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
