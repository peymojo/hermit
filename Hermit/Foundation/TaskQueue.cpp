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
		typedef std::pair<LockTaskQueueCompletionPtr, HermitPtr> LockCallback;
		typedef std::vector<LockCallback> LockCallbackVector;
		
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
			
			int32_t mLockCount;
			bool mLocked;
			LockCallbackVector mLockCallbacks;
			Queue mPendingTasks;
		};
		typedef std::shared_ptr<QueueInfoImpl> QueueInfoImplPtr;
		
		//
		void ThreadProc(QueueInfoPtr param) {
			auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(param);
			while (true) {
				pthread_mutex_lock(&queueInfo->mMutex);
				while (!queueInfo->mQuit &&
					   (queueInfo->mTasks.empty() || queueInfo->mBusy) &&
					   !(queueInfo->mTasks.empty() && !queueInfo->mBusy && (queueInfo->mLockCount > 0) && !queueInfo->mLocked)) {
					pthread_cond_wait(&queueInfo->mCondition, &queueInfo->mMutex);
				}
				if (queueInfo->mQuit) {
					pthread_mutex_unlock(&queueInfo->mMutex);
					break;
				}
				
				LockCallbackVector lockCallbacks;
				QueueEntryPtr nextTask;
				if ((queueInfo->mLockCount > 0) && !queueInfo->mLocked && queueInfo->mTasks.empty()) {
					queueInfo->mLocked = true;
					lockCallbacks.swap(queueInfo->mLockCallbacks);
				}
				else {
					nextTask = queueInfo->mTasks.front();
					queueInfo->mTasks.pop();
					queueInfo->mBusy = true;
				}
				pthread_mutex_unlock(&queueInfo->mMutex);
				
				if (nextTask != nullptr) {
					nextTask->mTask->PerformTask(nextTask->mH_);
				}
				else if (!lockCallbacks.empty()) {
					auto end = lockCallbacks.end();
					for (auto it = lockCallbacks.begin(); it != end; ++it) {
						(*it).first->Call((*it).second, LockTaskQueueResult::kSuccess);
					}
				}
			}
			
			pthread_mutex_lock(&queueInfo->mMutex);
			pthread_cond_signal(&queueInfo->mCondition);
			pthread_mutex_unlock(&queueInfo->mMutex);
			
			auto end = queueInfo->mLockCallbacks.end();
			for (auto it = queueInfo->mLockCallbacks.begin(); it != end; ++it) {
				(*it).first->Call((*it).second, LockTaskQueueResult::kCancel);
			}
			
			pthread_exit(nullptr);
		}
		
	} // namespace TaskQueue_Impl
	using namespace TaskQueue_Impl;
	
	//
	QueueInfoImpl::QueueInfoImpl() :
		mBusy(false),
		mQuit(false),
		mLockCount(0),
		mLocked(false) {
		
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
		
		if (queueInfo->mLockCount > 0) {
			queueInfo->mPendingTasks.push(entry);
		}
		else {
			queueInfo->mTasks.push(entry);
		}
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
	
	//
	void TaskQueue::Lock(const HermitPtr& h_, const LockTaskQueueCompletionPtr& completion) {
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		pthread_mutex_lock(&queueInfo->mMutex);
		
		bool callCallbackNow = false;
		queueInfo->mLockCount++;
		if (queueInfo->mLocked) {
			callCallbackNow = true;
		}
		else {
			queueInfo->mLockCallbacks.push_back(make_pair(completion, h_));
		}
		
		pthread_cond_signal(&queueInfo->mCondition);
		pthread_mutex_unlock(&queueInfo->mMutex);
		
		if (callCallbackNow) {
			completion->Call(h_, LockTaskQueueResult::kSuccess);
		}
	}
	
	//
	void TaskQueue::Unlock(const HermitPtr& h_) {
		auto queueInfo = std::static_pointer_cast<QueueInfoImpl>(mQueueInfo);
		pthread_mutex_lock(&queueInfo->mMutex);
		
		queueInfo->mLockCount--;
		if (queueInfo->mLockCount == 0) {
			queueInfo->mLocked = false;
			if (!queueInfo->mTasks.empty()) {
				NOTIFY_ERROR(h_, "TaskQueue::Unlock(): !queueInfo->mTasks.empty()");
			}
			queueInfo->mTasks.swap(queueInfo->mPendingTasks);
		}
		
		pthread_cond_signal(&queueInfo->mCondition);
		pthread_mutex_unlock(&queueInfo->mMutex);
	}
	
} // namespace hermit
