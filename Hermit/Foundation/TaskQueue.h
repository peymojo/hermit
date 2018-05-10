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

#ifndef TaskQueue_h
#define TaskQueue_h

#include "AsyncFunction.h"
#include "AsyncTaskQueue.h"
#include "Hermit.h"

namespace hermit {

	//
	class QueueInfo {
	public:
		virtual ~QueueInfo() = default;
	};
	typedef std::shared_ptr<QueueInfo> QueueInfoPtr;
	
	//
	class TaskQueue {
	public:
		//
		TaskQueue();
		
		//
		~TaskQueue();
		
		//
		bool QueueTask(const HermitPtr& h_, const AsyncTaskPtr& inTask);
		
		//
		void TaskComplete();
		
		//
		QueueInfoPtr mQueueInfo;
	};

} // namespace hermit

#endif
