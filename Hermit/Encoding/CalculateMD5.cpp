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

#include "Hermit/Encoding/CalculateMD5FromStream.h"
#include "CalculateMD5.h"

namespace hermit {
	namespace encoding {
		
		namespace {

			//
			class GetDataFunction : public DataProviderBlock {
			public:
				//
				GetDataFunction(const DataBuffer& data) : mData(data) {
				}
				
				//
				virtual void ProvideData(const HermitPtr& h_,
										 const DataHandlerBlockPtr& dataHandler,
										 const StreamResultBlockPtr& resultBlock) override {
					auto result = dataHandler->HandleData(h_, mData, true);
					resultBlock->Call(h_, result);
				}
				
				//
				DataBuffer mData;
			};
			
		} // private namespace
		
		//
		void CalculateMD5(const HermitPtr& h_, const DataBuffer& data, const CalculateHashCompletionPtr& completion) {
			auto getDataFunction = std::make_shared<GetDataFunction>(data);
			CalculateMD5FromStream(h_, getDataFunction, completion);
		}
		
	} // namespace encoding
} // namespace hermit
