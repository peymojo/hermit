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

#ifndef S3RetryClass_h
#define S3RetryClass_h

#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/Thread.h"
#include "S3Notification.h"

namespace hermit {
	namespace s3 {
		
		template <typename T>
		class RetryClassT {
		public:
			void AttemptWithRetry(const HermitPtr& h_, T& t) {
				int retries = 0;
				int sleepInterval = 1;
				int sleepIntervalStep = 1;
				typename T::ResultType result = T::kDefaultResult;
				while (true) {
					
					if (CHECK_FOR_ABORT(h_)) {
						t.Canceled();
						break;
					}
					
					if (retries > 0) {
						S3NotificationParams params(t.OpName(), retries, result);
						NOTIFY(h_, kS3RetryNotification, &params);
					}
					
					result = t.AttemptOnce(h_);
					if (!t.ShouldRetry(result)) {
						if (retries > 0) {
							S3NotificationParams params(t.OpName(), retries, result);
							NOTIFY(h_, kS3RetryCompleteNotification, &params);
						}
						t.ProcessResult(h_, result);
						break;
					}
					if (++retries == T::kMaxRetries) {
						S3NotificationParams params(t.OpName(), retries, result);
						NOTIFY(h_, kS3MaxRetriesExceededNotification, &params);
						t.RetriesExceeded(h_, result);
						break;
					}
					
					int fifthSecondIntervals = sleepInterval * 5;
					for (int i = 0; i < fifthSecondIntervals; ++i) {
						if (CHECK_FOR_ABORT(h_)) {
							t.Canceled();
							break;
						}
						Sleep(200);
					}
					sleepInterval += sleepIntervalStep;
					sleepIntervalStep += 2;
				}
			}
		};
		
	} // namespace
} // namespace

#endif
