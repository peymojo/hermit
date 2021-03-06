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

#ifndef S3Notification_h
#define S3Notification_h

#include <cstdint>
#include "S3Result.h"

namespace hermit {
namespace s3 {

//
extern const char* kS3RetryNotification;
extern const char* kS3RetryCompleteNotification;
extern const char* kS3MaxRetriesExceededNotification;

//
struct S3NotificationParams {
	//
	S3NotificationParams(const char* opName, std::int32_t count, S3Result result);
	//
	const char* mOpName;
	std::int32_t mCount;
	S3Result mResult;
};

} // namespace s3
} // namespace hermit

#endif
