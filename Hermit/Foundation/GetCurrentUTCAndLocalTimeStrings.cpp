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

#include <string.h>
#include <time.h>
#include "GetCurrentUTCAndLocalTimeStrings.h"

namespace hermit {
	
	//
	void GetCurrentUTCAndLocalTimeStrings(std::string& outUTCTimeString, std::string& outLocalTimeString) {
		time_t now = 0;
		time(&now);
		
		struct tm utcTime;
		memset(&utcTime, 0, sizeof(struct tm));
		gmtime_r(&now, &utcTime);
		
		char utcBuf[256];
		memset(utcBuf, 0, 256);
		strftime(utcBuf, 256, "%Y-%m-%d %H:%M:%S UTC", &utcTime);
		
		struct tm localTime;
		memset(&localTime, 0, sizeof(struct tm));
		localtime_r(&now, &localTime);
		
		char localBuf[256];
		memset(localBuf, 0, 256);
		strftime(localBuf, 256, "%Y-%m-%d %H:%M:%S %Z", &localTime);
		
		outUTCTimeString = utcBuf;
		outLocalTimeString = localBuf;
	}
	
} // namespace hermit
