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

#ifndef GetFileXAttrs_h
#define GetFileXAttrs_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		enum GetFileXAttrsResult
		{
			kGetFileXAttrsResult_Unknown,
			kGetFileXAttrsResult_Success,
			kGetFileXAttrsResult_AccessDenied,
			kGetFileXAttrsResult_Error
		};
		
		//
		//
		DEFINE_CALLBACK_3A(GetFileXAttrsCallback,
						   GetFileXAttrsResult,			// inResult
						   std::string,					// inOneXAttrName
						   std::string);				// inOneXAttrData
		
		//
		//
		template <class StringPairVectorT>
		class GetFileXAttrsCallbackClassT : public GetFileXAttrsCallback {
		public:
			//
			//
			GetFileXAttrsCallbackClassT() : mResult(kGetFileXAttrsResult_Unknown) {
			}
			
			//
			//
			bool Function(const GetFileXAttrsResult& inResult,
						  const std::string& inOneXAttrName,
						  const std::string& inOneXAttrData) {
				mResult = inResult;
				if (inResult == kGetFileXAttrsResult_Success) {
					auto value = std::make_pair(inOneXAttrName, inOneXAttrData);
					mXAttrs.push_back(value);
				}
				return true;
			}
			
			//
			//
			GetFileXAttrsResult mResult;
			StringPairVectorT mXAttrs;
		};

		//
		//
		void GetFileXAttrs(const HermitPtr& h_,
						   const FilePathPtr& inFilePath,
						   const GetFileXAttrsCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif
