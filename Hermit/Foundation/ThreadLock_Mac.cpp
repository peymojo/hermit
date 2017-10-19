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
#include "ThreadLock.h"

namespace hermit {

//
//
class MacThreadLockImpl
	:
	public ThreadLockImpl
{
public:
	//
	//
	MacThreadLockImpl()
	{
		pthread_mutexattr_t attr;
		::pthread_mutexattr_init(&attr);
		::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
		::pthread_mutex_init(&mMutex, &attr);
		::pthread_mutexattr_destroy(&attr);
	}
	
	//
	//
	virtual ~MacThreadLockImpl()
	{
		::pthread_mutex_destroy(&mMutex);
	}

protected:
	//
	//
	virtual void Enter()
	{
		::pthread_mutex_lock(&mMutex);	
	}

	//
	//
	virtual void Exit()
	{
		::pthread_mutex_unlock(&mMutex);
	}

private:
	//
	//
	pthread_mutex_t mMutex;
};

//
//
ThreadLock::ThreadLock()
	:
	mImpl(new MacThreadLockImpl)
{
}

//
//
ThreadLock::~ThreadLock()
{
	delete mImpl;
}

//
//
void ThreadLock::Enter()
{
	mImpl->Enter();
}

//
//
void ThreadLock::Exit()
{
	mImpl->Exit();
}

} // namespace hermit
