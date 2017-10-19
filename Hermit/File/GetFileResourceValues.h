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

#ifndef GetFileResourceValues_h
#define GetFileResourceValues_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_2A(GetFileResourceValuesCallback,
						   bool,							// inSuccess
						   std::string);					// inResourceValues
		
		//
		//
		template <typename StringT>
		class GetFileResourceValuesCallbackClass
		:
		public GetFileResourceValuesCallback
		{
		public:
			//
			//
			GetFileResourceValuesCallbackClass()
			:
			mSuccess(false)
			{
			}
			
			//
			//
			bool Function(const bool& inSuccess,
						  const std::string& inResourceValues)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mResourceValues = inResourceValues;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			StringT mResourceValues;
		};
		
		//
		//
		void GetFileResourceValues(const HermitPtr& h_,
								   const FilePathPtr& inFilePath,
								   const GetFileResourceValuesCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
