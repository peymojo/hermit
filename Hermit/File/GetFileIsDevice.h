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

#ifndef GetFileIsDevice_h
#define GetFileIsDevice_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/Hermit.h"
#include "FilePath.h"

namespace hermit {
	namespace file {
		
		//
		//
		DEFINE_CALLBACK_4A(
						   GetFileIsDeviceCallback,
						   bool,						// inSuccess
						   bool,						// inIsDevice
						   int32_t,					// inDeviceMode
						   int32_t);					// inDeviceID
		
		//
		//
		class GetFileIsDeviceCallbackClass
		:
		public GetFileIsDeviceCallback
		{
		public:
			//
			//
			GetFileIsDeviceCallbackClass()
			:
			mSuccess(false),
			mIsDevice(false),
			mDeviceMode(0),
			mDeviceID(0)
			{
			}
			
			//
			//
			bool Function(
						  const bool& inSuccess,
						  const bool& inIsDevice,
						  const int32_t& inDeviceMode,
						  const int32_t& inDeviceID)
			{
				mSuccess = inSuccess;
				if (inSuccess)
				{
					mIsDevice = inIsDevice;
					mDeviceMode = inDeviceMode;
					mDeviceID = inDeviceID;
				}
				return true;
			}
			
			//
			//
			bool mSuccess;
			bool mIsDevice;
			int32_t mDeviceMode;
			int32_t mDeviceID;
		};
		
		//
		//
		void GetFileIsDevice(const HermitPtr& h_,
							 const FilePathPtr& inFilePath,
							 const GetFileIsDeviceCallbackRef& inCallback);
		
	} // namespace file
} // namespace hermit

#endif

