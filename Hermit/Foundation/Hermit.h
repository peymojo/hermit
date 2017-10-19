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

#ifndef Hermit_h
#define Hermit_h

#include <memory>

namespace hermit {
	
	//
	struct Hermit {
		//
		virtual ~Hermit() = default;
		
		//
		virtual bool ShouldAbort() = 0;
		
		//
		virtual void Notify(const char* name, const void* param) = 0;
	};
	typedef std::shared_ptr<Hermit> HermitPtr;
	
} // namespace hermit

// utility macro to call the notification function, or do nothing if the function is null
#define NOTIFY(h_, name, param) { if (h_ != nullptr) { h_->Notify(name, param); } }

// utility macro to check for abort, evaluates true if caller should abort
#define CHECK_FOR_ABORT(h_) ((h_ != nullptr) && h_->ShouldAbort())

#endif /* Hermit_h */
