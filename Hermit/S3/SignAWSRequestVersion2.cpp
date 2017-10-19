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

#include <string>
#include "Hermit/Encoding/BinaryToBase64.h"
#include "Hermit/Encoding/CalculateHMACSHA1.h"
#include "SignAWSRequestVersion2.h"

namespace hermit {
	namespace s3 {
		
		//
		void SignAWSRequestVersion2(const std::string& inAWSPublicKey,
									const std::string& inAWSPrivateKey,
									const std::string& inMethod,
									const std::string& inMD5String,
									const std::string& inContentType,
									const std::string& inOptionalExtraParam,
									const std::string& inURL,
									const SignAWSRequestVersion2CallbackRef& inCallback) {
			time_t now;
			time(&now);
			tm localTime;
			localtime_r(&now, &localTime);
			char dateBuf[2048];
			strftime(dateBuf, 2048, "%a, %d %b %Y %H:%M:%S %Z", &localTime);
			std::string date(dateBuf);
			
			std::string stringToSign(inMethod);
			stringToSign += "\n";
			stringToSign += inMD5String;
			stringToSign += "\n";
			stringToSign += inContentType;
			stringToSign += "\n";
			stringToSign += date;
			stringToSign += "\n";
			if (!inOptionalExtraParam.empty()) {
				stringToSign += inOptionalExtraParam;
				stringToSign += "\n";
			}
			stringToSign += inURL;
			
			std::string signature;
			encoding::CalculateHMACSHA1(inAWSPrivateKey, stringToSign, signature);
			encoding::BinaryToBase64(signature, signature);
			
			std::string awsAuthorization("AWS ");
			awsAuthorization += inAWSPublicKey;
			awsAuthorization += ":";
			awsAuthorization += signature;
			
			inCallback.Call(true, awsAuthorization, date);
		}
		
	} // namespace s3
} // namespace hermit
