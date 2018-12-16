//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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

#include <sstream>
#include <sys/sysctl.h>
#include <unistd.h>
#include "StaticLog.h"
#include "IsDebuggerActive.h"

namespace hermit {

	//
	bool IsDebuggerActive() {
#ifdef DEBUG
		struct kinfo_proc info;
		info.kp_proc.p_flag = 0;
		
		int mib[4];
		mib[0] = CTL_KERN;
		mib[1] = KERN_PROC;
		mib[2] = KERN_PROC_PID;
		mib[3] = getpid();
		
		size_t size = sizeof(info);
		int result = sysctl(mib, sizeof(mib) / sizeof(*mib), &info, &size, NULL, 0);
		if (result == -1) {
			std::ostringstream strm;
			strm << "IsDebuggerActive: sysctl failed, errno: " << errno;
			StaticLog(strm.str().c_str());
			return false;
		}
		
		return ( (info.kp_proc.p_flag & P_TRACED) != 0 );
#else
		return false;
#endif
	}

} // namespace hermit
