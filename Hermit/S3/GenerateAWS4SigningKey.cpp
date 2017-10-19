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
#include <sys/time.h>
#include "Hermit/Encoding/CalculateHMACSHA256.h"
#include "GenerateAWS4SigningKey.h"

namespace hermit {
	namespace s3 {
		
		//
		void GenerateAWS4SigningKey(const std::string& inAWSPrivateKey,
									const std::string& inRegion,
									const GenerateAWS4SigningKeyCallbackRef& inCallback) {
			time_t now;
			time(&now);
			tm globalTime;
			gmtime_r(&now, &globalTime);
			char dateBuf[32];
			strftime(dateBuf, 32, "%Y%m%d", &globalTime);
			std::string date(dateBuf);
			
			std::string inputKey("AWS4");
			inputKey += inAWSPrivateKey;
			
			std::string signingKey;
			encoding::CalculateHMACSHA256(inputKey, date, signingKey);
			encoding::CalculateHMACSHA256(signingKey, inRegion, signingKey);
			std::string service("s3");
			encoding::CalculateHMACSHA256(signingKey, service, signingKey);
			std::string scopeAnchor("aws4_request");
			encoding::CalculateHMACSHA256(signingKey, scopeAnchor, signingKey);
			
			inCallback.Call(signingKey, date);
		}
		
	} // namespace s3
} // namespace hermit
