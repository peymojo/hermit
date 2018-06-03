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
#include "GetCurrentUTCTimeString.h"

namespace hermit {
	
	//
	void GetCurrentUTCTimeString(const HermitPtr& h_, std::string& outTimeString) {
		time_t now = 0;
		time(&now);
		
		struct tm utcTime;
		memset(&utcTime, 0, sizeof(struct tm));
		gmtime_r(&now, &utcTime);
		
		char buf[256];
		memset(buf, 0, 256);
		strftime(buf, 256, "%Y-%m-%d %H:%M:%S UTC", &utcTime);
		outTimeString = buf;
	}
	
} // namespace hermit
