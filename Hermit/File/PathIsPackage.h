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

#ifndef PathIsPackage_h
#define PathIsPackage_h

#include "Hermit/Foundation/Callback.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_3A(
						   PathIsPackageCallback,
						   FilePathPtr,				// inPath
						   bool,						// inSuccess
						   bool);						// inIsPackage
		
		//
		//
		class PathIsPackageCallbackClass
		:
		public PathIsPackageCallback
		{
		public:
			//
			//
			PathIsPackageCallbackClass()
			:
			mSuccess(false),
			mIsPackage(false)
			{
			}
			
			//
			//
			bool Function(
						  const FilePathPtr& inFilePath,
						  const bool& inSuccess,
						  const bool& inIsPackage)
			{
				mSuccess = inSuccess;
				mIsPackage = inIsPackage;
				return true;
			}
			
			//
			//
			bool mSuccess;
			bool mIsPackage;
		};
		
		//
		//
		void PathIsPackage(const HermitPtr& h_,
						   const FilePathPtr& inPath,
						   const PathIsPackageCallbackRef& inCallback);
		
	} // namespace file
} // namesapce lib

#endif 
