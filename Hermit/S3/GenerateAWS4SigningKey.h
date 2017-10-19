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

#ifndef GenerateAWS4SigningKey_h
#define GenerateAWS4SigningKey_h

#include <string>
#include "Hermit/Foundation/Callback.h"

namespace hermit {
namespace s3 {

//
//
DEFINE_CALLBACK_2A(
	GenerateAWS4SigningKeyCallback,
	std::string,						// inKey
	std::string);						// inDateString

//
//
class GenerateAWS4SigningKeyCallbackClass
	:
	public GenerateAWS4SigningKeyCallback
{
public:
	//
	//
	GenerateAWS4SigningKeyCallbackClass()
	{
	}
	
	//
	//
	bool Function(
		const std::string& inKey,
		const std::string& inDateString)
	{
		mKey.assign(inKey);
		mDateString = inDateString;
		return true;
	}
	
	//
	//
	std::string mKey;
	std::string mDateString;
};

//
//
void GenerateAWS4SigningKey(
	const std::string& inAWSPrivateKey,
	const std::string& inRegion,
	const GenerateAWS4SigningKeyCallbackRef& inCallback);
	
} // namespace s3
} // namespace hermit

#endif
