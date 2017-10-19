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

#ifndef SignAWSRequestVersion2_h
#define SignAWSRequestVersion2_h

#include "Hermit/Foundation/Callback.h"

namespace hermit {
namespace s3 {

//
//
DEFINE_CALLBACK_3A(
	SignAWSRequestVersion2Callback,
	bool,							// inSuccess
	std::string,					// inAuthorizationString
	std::string);					// inDateString

//
//
class SignAWSRequestVersion2CallbackClass
	:
	public SignAWSRequestVersion2Callback
{
public:
	//
	//
	SignAWSRequestVersion2CallbackClass()
		:
		mSuccess(false)
	{
	}
	
	//
	//
	bool Function(
		const bool& inSuccess,
		const std::string& inAuthorizationString,
		const std::string& inDateString)
	{
		mSuccess = inSuccess;
		if (inSuccess)
		{
			mAuthorizationString = inAuthorizationString;
			mDateString = inDateString;
		}
		return true;
	}
	
	//
	//
	bool mSuccess;
	std::string mAuthorizationString;
	std::string mDateString;
};

//
//
void SignAWSRequestVersion2(
	const std::string& inAWSPublicKey,
	const std::string& inAWSPrivateKey,
	const std::string& inMethod,
	const std::string& inMD5String,
	const std::string& inContentType,
	const std::string& inOptionalExtraParam,
	const std::string& inURL,
	const SignAWSRequestVersion2CallbackRef& inCallback);
	
} // namespace s3
} // namespace hermit

#endif
