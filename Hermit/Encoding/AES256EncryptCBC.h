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

#ifndef AES256EncryptCBC_h
#define AES256EncryptCBC_h

#include <string>
#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/DataBuffer.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace encoding {
		
		//
		//
		enum AES256EncryptCBCStatus
		{
			kAES256EncryptCBC_Unknown,
			kAES256EncryptCBC_Success,
			kAES256EncryptCBC_Canceled,
			kAES256EncryptCBC_Error
		};
		
		//
		//
		DEFINE_CALLBACK_2A(AES256EncryptCBCCallback,
						   AES256EncryptCBCStatus,			// inStatus
						   DataBuffer);						// inData
		
		//
		//
		class AES256EncryptCBCCallbackClass
		:
		public AES256EncryptCBCCallback
		{
		public:
			//
			//
			AES256EncryptCBCCallbackClass()
			:
			mStatus(kAES256EncryptCBC_Unknown)
			{
			}
			
			//
			//
			bool Function(const AES256EncryptCBCStatus& inStatus,
						  const DataBuffer& inData)
			{
				mStatus = inStatus;
				if (inStatus == kAES256EncryptCBC_Success)
				{
					mValue.assign(inData.first, inData.second);
				}
				return true;
			}
			
			//
			//
			AES256EncryptCBCStatus mStatus;
			std::string mValue;
		};
		
		//
		//
		void AES256EncryptCBC(const HermitPtr& h_,
							  const DataBuffer& inPlainText,
							  const std::string& inKey,
							  const std::string& inInputVector,
							  const AES256EncryptCBCCallbackRef& inCallback);
		
	} // namespace encoding
} // namespace hermit

#endif
