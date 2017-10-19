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

#ifndef ThreadLock_h
#define ThreadLock_h

namespace hermit {

//
//
class ThreadLockImpl
{
public:
	//
	//
	virtual ~ThreadLockImpl()
	{
	}
	
	//
	//
	virtual void Enter() = 0;

	//
	//
	virtual void Exit() = 0;
};

//
//
class ThreadLock
{
public:
	//
	//
	ThreadLock();

	//
	//
	~ThreadLock();

	//
	//
	void Enter();

	//
	//
	void Exit();

private:
	//
	//
	ThreadLock(
		const ThreadLock& inOther);
		
	//
	//
	ThreadLockImpl* mImpl;
};

//
//
class ThreadLockScope
{
public:
	//
	//
	ThreadLockScope(
		ThreadLock& inLock)
		:
		mLock(inLock)
	{
		mLock.Enter();
	}

	//
	//
	~ThreadLockScope()
	{
		mLock.Exit();
	}

private:
	//
	//
	ThreadLock& mLock;
};

} // namespace hermit

#endif
