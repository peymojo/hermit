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

#ifndef FileExists_h
#define FileExists_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_2A(
						   FileExistsCallback,
						   bool,					// inSuccess
						   bool);					// inExists
		
		//
		//
		class FileExistsCallbackClass
		:
		public FileExistsCallback
		{
		public:
			//
			//
			FileExistsCallbackClass()
			:
			mSuccess(false),
			mExists(false)
			{
			}
			
			//
			//
			bool Function(
						  const bool& inSuccess,
						  const bool& inExists)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mExists = inExists;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			bool mExists;
		};
		
		//
		void FileExists(const HermitPtr& h_,
						const FilePathPtr& inFilePath,
						const FileExistsCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
