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

#include <queue>
#include <thread>
#include <vector>
#include "Notification.h"
#include "StaticLog.h"
#include "TaskQueue.h"

namespace hermit {
	namespace TaskQueue_Impl {
		
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
		
		//
		class QueueInfoImpl : public QueueInfo {
		public:
			//
			QueueInfoImpl();
			
			//
			void Shutdown();
			
			//
			Queue mTasks;
			std::thread mThread;
			pthread_attr_t mAttr;
			pthread_mutex_t mMutex;
			pthread_cond_t mCondition;
			bool mBusy;
			bool mQuit;
		};
		typedef std::shared_ptr<QueueInfoImpl> QueueInfoImplPtr;
		
		//
		void ThreadProc(QueueInfoPtr param) {
			auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(param);
			while (true) {
				pthread_mutex_lock(&queueInfo->mMutex);
				while (!queueInfo->mQuit && (queueInfo->mTasks.empty() || queueInfo->mBusy)) {
					pthread_cond_wait(&queueInfo->mCondition, &queueInfo->mMutex);
				}
				if (queueInfo->mQuit) {
					pthread_mutex_unlock(&queueInfo->mMutex);
					break;
				}
				
				QueueEntryPtr nextTask = queueInfo->mTasks.front();
				queueInfo->mTasks.pop();
				queueInfo->mBusy = true;
				pthread_mutex_unlock(&queueInfo->mMutex);
				
				if (nextTask != nullptr) {
					nextTask->mTask->PerformTask(nextTask->mH_);
				}
			}
			
			pthread_mutex_lock(&queueInfo->mMutex);
			pthread_cond_signal(&queueInfo->mCondition);
			pthread_mutex_unlock(&queueInfo->mMutex);
			
			pthread_exit(nullptr);
		}
		
	} // namespace TaskQueue_Impl
	using namespace TaskQueue_Impl;
	
	//
	QueueInfoImpl::QueueInfoImpl() :
		mBusy(false),
		mQuit(false) {
		
		pthread_mutex_init(&mMutex, nullptr);
		pthread_cond_init(&mCondition, nullptr);
		pthread_attr_init(&mAttr);
		pthread_attr_setdetachstate(&mAttr, PTHREAD_CREATE_JOINABLE);
	}
	
	//
	void QueueInfoImpl::Shutdown() {
		pthread_mutex_lock(&mMutex);
		
		if (!mTasks.empty()) {
			StaticLog("QueueInfoImpl::Shutdown(): !mTasks.empty()");
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
	TaskQueue::TaskQueue() :
		mQueueInfo(std::make_shared<QueueInfoImpl>()) {
		
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		queueInfo->mThread = std::thread(ThreadProc, mQueueInfo);
	}
	
	//
	TaskQueue::~TaskQueue() {
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		queueInfo->Shutdown();
	}
	
	//
	bool TaskQueue::QueueTask(const HermitPtr& h_, const AsyncTaskPtr& task) {
		QueueEntryPtr entry(new QueueEntry(h_, task));				
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		pthread_mutex_lock(&queueInfo->mMutex);
		queueInfo->mTasks.push(entry);
		pthread_cond_signal(&queueInfo->mCondition);
		pthread_mutex_unlock(&queueInfo->mMutex);
		
		return true;
	}
	
	//
	void TaskQueue::TaskComplete() {
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		pthread_mutex_lock(&queueInfo->mMutex);
				
		queueInfo->mBusy = false;
		pthread_cond_signal(&queueInfo->mCondition);
		pthread_mutex_unlock(&queueInfo->mMutex);
	}
	
} // namespace hermit
