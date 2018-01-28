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

#include <map>
#include <pthread.h>
#include <queue>
#include <string>
#include "AsyncTaskQueue.h"
#include "Hermit.h"
#include "StaticLog.h"
#include "ThreadLock.h"

namespace hermit {
	namespace AsyncTaskQueue_Impl {
		
		//
		class QueueEntry {
		public:
			//
			QueueEntry(const HermitPtr& h_, const AsyncTaskPtr& inTask) : mH_(h_), mTask(inTask) {
			}
			
			//
			~QueueEntry() = default;
			
			//
			HermitPtr mH_;
			AsyncTaskPtr mTask;
		};
		typedef std::shared_ptr<QueueEntry> QueueEntryPtr;
		
		//
		typedef std::queue<QueueEntryPtr> Queue;
		
		//
		typedef std::shared_ptr<Queue> QueuePtr;
		
		//
		typedef std::multimap<int32_t, QueuePtr> TaskQueueMap;
		
		//
		static ThreadLock sTasksLock;
		static bool sThreadsStarted = false;
		static bool sQuitThreads = false;
		static TaskQueueMap sTasks;
		static pthread_mutex_t sMutex;
		static pthread_cond_t sCondition;
		static pthread_attr_t sAttr;
		
#define THREAD_COUNT 8
		
		static pthread_t sThreads[THREAD_COUNT];
		
		//
		void* StartThread(void* inParam) {
			while (true) {
				QueueEntryPtr nextTask;
				pthread_mutex_lock(&sMutex);
				while (!sQuitThreads && sTasks.empty()) {
					pthread_cond_wait(&sCondition, &sMutex);
				}
				if (sQuitThreads) {
					pthread_mutex_unlock(&sMutex);
					break;
				}
				auto it = sTasks.begin();
				nextTask = it->second->front();
				it->second->pop();
				if (it->second->empty()) {
					sTasks.erase(it);
				}
				pthread_mutex_unlock(&sMutex);
				
				nextTask->mTask->PerformTask(nextTask->mH_);
			}
			
			pthread_mutex_lock(&sMutex);
			pthread_cond_signal(&sCondition);
			pthread_mutex_unlock(&sMutex);
			pthread_exit(nullptr);
		}
		
	} // namespace AsyncTaskQueue_Impl
	using namespace AsyncTaskQueue_Impl;
	
	//
	bool QueueAsyncTask(const HermitPtr& h_, const AsyncTaskPtr& task, const int32_t& priority) {
		bool queueHasBeenShutDown = false;
		QueueEntryPtr entry(new QueueEntry(h_, task)); {
			ThreadLockScope lock(sTasksLock);
			if (sQuitThreads) {
				queueHasBeenShutDown = true;
			}
			else if (!sThreadsStarted) {
				pthread_mutex_init(&sMutex, nullptr);
				pthread_cond_init(&sCondition, nullptr);
				
				pthread_attr_init(&sAttr);
				pthread_attr_setdetachstate(&sAttr, PTHREAD_CREATE_JOINABLE);
				
				for (int n = 0; n < THREAD_COUNT; ++n) {
					pthread_create(&sThreads[n], &sAttr, StartThread, (void*)0);
				}
				
				sThreadsStarted = true;
			}
		}
		
		if (queueHasBeenShutDown) {
			return false;
		}
		
		pthread_mutex_lock(&sMutex);
		auto it = sTasks.find(priority);
		if (it == end(sTasks)) {
			auto queue = std::make_shared<Queue>();
			queue->push(entry);
			sTasks.insert(TaskQueueMap::value_type(priority, queue));
		}
		else {
			it->second->push(entry);
		}
		pthread_cond_signal(&sCondition);
		pthread_mutex_unlock(&sMutex);
		
		return true;
	}
	
	//
	void ShutdownAsyncTaskQueue() {
		{
			ThreadLockScope lock(sTasksLock);
			if (sThreadsStarted) {
				sThreadsStarted = false;
				if (!sTasks.empty()) {
					StaticLog("ShutdownAsyncTaskQueue: !mTasks.empty()");
				}
				
				pthread_mutex_lock(&sMutex);
				sTasks.clear();
				sQuitThreads = true;
				pthread_cond_signal(&sCondition);
				pthread_mutex_unlock(&sMutex);
			}
		}
		
		for (int n = 0; n < THREAD_COUNT; ++n) {
			pthread_join(sThreads[n], nullptr);
		}
	}
	
} // namespace hermit

