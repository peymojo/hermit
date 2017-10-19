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

#ifndef Notification_h
#define Notification_h

#include <sstream>
#include "Hermit.h"

namespace hermit {
	
	//
	extern const char* kMessageNotification;
	//
	enum class MessageSeverity {
		kInfo,
		kWarning,
		kError
	};
	//
	struct MessageParams {
		//
		MessageParams(MessageSeverity sev, const char* m) : severity(sev), message(m) {
		}
		//
		MessageSeverity severity;
		const char* message;
	};
	
	//
	template <class A>
	void StreamOut(const HermitPtr& h_, std::ostream& strm, A arg) {
		strm << arg;
	}
	
	//
	template <class A, class... B>
	void StreamOut(const HermitPtr& h_, std::ostream& strm, A arg1, B... rest) {
		strm << arg1 << " ";
		StreamOut(h_, strm, rest...);
	}
	
	//
	void NotifyMessage(const HermitPtr& h_, MessageSeverity severity, const char* message);
	
	//
	template <class... A>
	void NotifyMessage(const HermitPtr& h_, MessageSeverity severity, A... args) {
		if (h_ != nullptr) {
			std::stringstream strm;
			StreamOut(h_, strm, args...);
			NotifyMessage(h_, severity, strm.str().c_str());
		}
	}
	
} // namespace hermit

// info & warning do not include the calling function name. error does
#define NOTIFY_INFO(h_, ...) hermit::NotifyMessage(h_, hermit::MessageSeverity::kInfo, __VA_ARGS__)
#define NOTIFY_WARNING(h_, ...) hermit::NotifyMessage(h_, hermit::MessageSeverity::kWarning, __VA_ARGS__)
#define NOTIFY_ERROR(h_, ...) hermit::NotifyMessage(h_, hermit::MessageSeverity::kError, __func__, __VA_ARGS__)

#endif
