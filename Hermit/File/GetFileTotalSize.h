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

#ifndef GetFileTotalSize_h
#define GetFileTotalSize_h

#include "Hermit/Foundation/HermitTypes.h"
#include "Hermit/Foundation/HermitCallback.h"
#include "FilePath.h"

namespace hermit
{

//
//
HERMIT_DEFINE_CALLBACK_2A(
	GetFileTotalSizeCallback,
	bool,						// inSuccess
	uint64_t);					// inTotalSize

//
//
class GetFileTotalSizeCallbackClass
	:
	public GetFileTotalSizeCallback
{
public:
	//
	//
	GetFileTotalSizeCallbackClass()
		:
		mSuccess(false),
		mTotalSize(0)
	{
	}
	
	//
	//
	bool Function(
		const ConstHubPtr& inHub,
		const bool& inSuccess,
		const uint64_t& inTotalSize)
	{
		mSuccess = inSuccess;
		if (inSuccess)
		{
			mTotalSize = inTotalSize;
		}
		return true;
	}
	
	//
	//
	bool mSuccess;
	uint64_t mTotalSize;
};

//
//
void GetFileTotalSize(
	const ConstHubPtr& inHub,
	const FilePathPtr& inFilePath,
	const GetFileTotalSizeCallbackRef& inCallback);
	
} // namespace hermit

#endif 
