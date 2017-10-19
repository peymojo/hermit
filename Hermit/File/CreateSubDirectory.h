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

#ifndef CreateSubDirectory_h
#define CreateSubDirectory_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_2A(CreateSubDirectoryCallback,
						   bool,							// inSuccess
						   FilePathPtr);					// inDirectoryPath
		
		//
		//
		template <typename T>
		class CreateSubDirectoryCallbackClassT
		{
		public:
			//
			//
			CreateSubDirectoryCallbackClassT()
			:
			mSuccess(false)
			{
			}
			
			//
			//
			bool Function(bool inSuccess, FilePathPtr inFilePath)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mFilePath = inFilePath;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			T mFilePath;
		};
		
		//
		void CreateSubDirectory(const HermitPtr& h_,
								const FilePathPtr& inBasePath,
								const std::string& inDirectoryName,
								const CreateSubDirectoryCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
