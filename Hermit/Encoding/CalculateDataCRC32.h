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

#ifndef CalculateDataCRC32_h
#define CalculateDataCRC32_h

#include <cstdlib>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/StreamDataFunction.h"

namespace hermit {
	namespace encoding {
		
		//
		enum class CalculateDataCRC32Result {
			kUnknown,
			kSuccess,
			kCanceled,
			kError
		};
		//
		DEFINE_ASYNC_FUNCTION_3A(CalculateDataCRC32Completion, HermitPtr, CalculateDataCRC32Result, std::uint32_t);
		
		//
		void CalculateDataCRC32(const HermitPtr& h_,
								const DataProviderPtr& dataProvider,
								const CalculateDataCRC32CompletionPtr& completion);
		
	} // namespace encoding
} // namespace hermit

#endif
