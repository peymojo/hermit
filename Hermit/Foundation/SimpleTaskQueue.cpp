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

#include <queue>
#include <thread>
#include <vector>
#include "StaticLog.h"
#include "SimpleTaskQueue.h"

namespace hermit {
	namespace SimpleTaskQueue_Impl {
		
		//
		class QueueEntry {
		public:
			//
			QueueEntry(const HermitPtr& h_, const AsyncTaskPtr& task) : mH_(h_), mTask(task) {
			}
			
			//
			HermitPtr mH_;
			AsyncTaskPtr mTask;
		};
		typedef std::shared_ptr<QueueEntry> QueueEntryPtr;
		
		//
		typedef std::queue<QueueEntryPtr> Queue;
		
	} // namespace SimpleTaskQueue_Impl
	using namespace SimpleTaskQueue_Impl;
	
	//
	class SimpleTaskQueueImpl {
	public:
		//
		SimpleTaskQueueImpl() : mQuit(false) {
			pthread_mutex_init(&mMutex, nullptr);
			pthread_cond_init(&mCondition, nullptr);
			pthread_attr_init(&mAttr);
			pthread_attr_setdetachstate(&mAttr, PTHREAD_CREATE_JOINABLE);
		}
		
		//
		void Shutdown() {
			pthread_mutex_lock(&mMutex);
			if (!mTasks.empty()) {
				StaticLog("SimpleTaskQueueImpl::Shutdown(): !mTasks.empty()");
				while (!mTasks.empty()) {
					mTasks.pop();
				}
			}
			mQuit = true;
			pthread_cond_signal(&mCondition);
			pthread_mutex_unlock(&mMutex);
			
			mThread.join();
		}

		//
		Queue mTasks;
		std::thread mThread;
		pthread_attr_t mAttr;
		pthread_mutex_t mMutex;
		pthread_cond_t mCondition;
		bool mQuit;
	};
	
	//
	void SimpleTaskQueueThreadProc(SimpleTaskQueueImplPtr impl) {
		while (true) {
			pthread_mutex_lock(&impl->mMutex);
			while (!impl->mQuit && impl->mTasks.empty()) {
				pthread_cond_wait(&impl->mCondition, &impl->mMutex);
			}
			if (impl->mQuit) {
				pthread_mutex_unlock(&impl->mMutex);
				break;
			}
			
			QueueEntryPtr nextTask = impl->mTasks.front();
			impl->mTasks.pop();
			pthread_mutex_unlock(&impl->mMutex);
			
			nextTask->mTask->PerformTask(nextTask->mH_);
		}
		
		pthread_mutex_lock(&impl->mMutex);
		pthread_cond_signal(&impl->mCondition);
		pthread_mutex_unlock(&impl->mMutex);
		
		pthread_exit(nullptr);
	}

	//
	SimpleTaskQueue::SimpleTaskQueue() :
	mImpl(std::make_shared<SimpleTaskQueueImpl>()) {
		mImpl->mThread = std::thread(SimpleTaskQueueThreadProc, mImpl);
	}
	
	//
	SimpleTaskQueue::~SimpleTaskQueue() {
		mImpl->Shutdown();
	}
	
	//
	bool SimpleTaskQueue::QueueTask(const HermitPtr& h_, const AsyncTaskPtr& task) {
		QueueEntryPtr entry(new QueueEntry(h_, task));
		pthread_mutex_lock(&mImpl->mMutex);
		mImpl->mTasks.push(entry);
		pthread_cond_signal(&mImpl->mCondition);
		pthread_mutex_unlock(&mImpl->mMutex);
		return true;
	}
	
} // namespace hermit
