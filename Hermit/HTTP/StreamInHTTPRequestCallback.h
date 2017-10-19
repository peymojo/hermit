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

#ifndef StreamInHTTPRequestCallback_h
#define StreamInHTTPRequestCallback_h

#include "Hermit/Foundation/Callback.h"
#include "Hermit/Foundation/EnumerateStringValuesFunction.h"
#include "HTTPRequestResult.h"

namespace hermit {
namespace http {

//
//
DEFINE_CALLBACK_3A(
	StreamInHTTPRequestCallback,
	HTTPRequestResult,							// inResult
	int,										// inResponseHTTPStatusCode
	EnumerateStringValuesFunctionRef);			// inResponseHeaderParamsFunction

//
//
template <typename StringPairVectorT>
class StreamInHTTPRequestCallbackClassT
	:
	public StreamInHTTPRequestCallback
{
public:
	//
	//
	StreamInHTTPRequestCallbackClassT()
		:
		mResult(HTTPRequestResult::kUnknown),
		mResponseStatusCode(0)
	{
	}
			
	//
	//
	bool Function(
		const HTTPRequestResult& inResult,
		const int& inResponseStatusCode,
		const EnumerateStringValuesFunctionRef& inParamsFunction)
	{
		mResult = inResult;
		if (inResult == HTTPRequestResult::kSuccess)
		{
			mResponseStatusCode = inResponseStatusCode;
			if (!inParamsFunction.IsNull())
			{
				EnumerateStringValuesCallbackClass responseParams;
				if (!inParamsFunction.Call(responseParams))
				{
					return false;
				}
				mParams.swap(responseParams.mValues);
			}
		}
		return true;
	}

	//
	//
	HTTPRequestResult mResult;
	int mResponseStatusCode;
	StringPairVectorT mParams;
};

} // namespace http
} // namespace hermit

#endif
