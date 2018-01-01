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

#ifndef StreamInS3RequestWithBody_h
#define StreamInS3RequestWithBody_h

#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/SharedBuffer.h"
#include "Hermit/Foundation/StreamDataFunction.h"
#include "S3ParamVector.h"
#include "StreamInS3RequestCompletion.h"

namespace hermit {
	namespace s3 {
		
		//
		void StreamInS3RequestWithBody(const HermitPtr& h_,
									   const std::string& url,
									   const std::string& method,
									   const S3ParamVector& params,
									   const SharedBufferPtr& body,
									   const DataHandlerBlockPtr& dataHandler,
									   const StreamInS3RequestCompletionPtr& completion);
		
	} // namespace s3
} // namespace hermit

#endif 
