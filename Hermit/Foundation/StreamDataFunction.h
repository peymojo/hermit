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

#ifndef StreamDataFunction_h
#define StreamDataFunction_h

#include "DataBuffer.h"
#include "Hermit.h"

namespace hermit {
	
	//
	enum class StreamDataResult {
		kUnknown,
		kSuccess,
		kCanceled,
		kDiskFull,
		kNoSuchFile,
		kError
	};

	//
	class StreamResultBlock {
	public:
		//
		virtual void Call(const HermitPtr& h_, StreamDataResult result) = 0;
	};
	typedef std::shared_ptr<StreamResultBlock> StreamResultBlockPtr;

	//
	class DataHandlerBlock {
	public:
		//
		virtual StreamDataResult HandleData(const HermitPtr& h_, const DataBuffer& data, bool isEndOfData) = 0;
	};
	typedef std::shared_ptr<DataHandlerBlock> DataHandlerBlockPtr;

	//
	class DataProviderBlock {
	public:
		//
		virtual void ProvideData(const HermitPtr& h_,
								 const DataHandlerBlockPtr& dataHandler,
								 const StreamResultBlockPtr& resultBlock) = 0;
	};
	typedef std::shared_ptr<DataProviderBlock> DataProviderBlockPtr;

	//
	class DataProviderClientBlock {
	public:
		//
		virtual void WithDataProvider(const HermitPtr& h_,
									  const DataProviderBlockPtr& dataProvider,
									  const StreamResultBlockPtr& resultBlock) = 0;
	};
	typedef std::shared_ptr<DataProviderClientBlock> DataProviderClientBlockPtr;
	
} // namespace hermit

#endif
