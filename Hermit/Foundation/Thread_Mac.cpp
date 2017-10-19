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

#include <pthread.h>
#include "Thread.h"

namespace hermit {

//
//
struct ThreadInfo
{
	ThreadProc mProc;
	void* mParam;
};

//
//
static void* Proc(
	void* inParam)
{
	if (inParam != nullptr)
	{
		ThreadInfo* info = reinterpret_cast<ThreadInfo*>(inParam);
		info->mProc(info->mParam);
	}
	return 0;
}

//
//
bool StartThread(
	ThreadProc inThreadProc,
	void* inThreadParam)
{
	ThreadInfo* info = new ThreadInfo;
	info->mProc = inThreadProc;
	info->mParam = inThreadParam;
	pthread_t id = 0;
	int result = ::pthread_create(&id, nullptr, Proc, info);
	return (result == 0);
}

//
//
void Sleep(
	int inMilliseconds)
{
	timespec t = { inMilliseconds / 1000, (inMilliseconds % 1000) * 1000000 };
	::nanosleep(&t, nullptr);
}

} // namespace hermit

