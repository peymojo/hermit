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

#ifndef AsyncTaskQueue_h
#define AsyncTaskQueue_h

#include <memory>
#include "Hermit.h"

namespace hermit {

	//
	class AsyncTask {
	public:
		//
		AsyncTask() {
		}
		
		//
		virtual ~AsyncTask() {
		}
		
		//
		virtual void PerformTask(const HermitPtr& h_) = 0;
	};
	typedef std::shared_ptr<AsyncTask> AsyncTaskPtr;
		
	//
	bool QueueAsyncTask(const HermitPtr& h_, const AsyncTaskPtr& task, const int32_t& priority);

	//
	void ShutdownAsyncTaskQueue();

} // namespace hermit

#endif
