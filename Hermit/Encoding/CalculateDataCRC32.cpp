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

#include "Hermit/Foundation/Notification.h"
#include "CalculateDataCRC32.h"
#include "CRC32.h"

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//
			class Calculator : public DataHandlerBlock {
			public:
				//
				Calculator() : mCRC(0xffffffff) {
				}
				
				//
				virtual void HandleData(const HermitPtr& h_,
										const DataBuffer& data,
										bool isEndOfData,
										const StreamResultBlockPtr& resultBlock) override {
					mCRC = UpdateCRC32(mCRC, data.first, data.second);
					resultBlock->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				uint32_t mCRC;
			};
			typedef std::shared_ptr<Calculator> CalculatorPtr;
			
			//
			class Completion : public StreamResultBlock {
			public:
				//
				Completion(const CalculatorPtr& calculator, const CalculateDataCRC32CompletionPtr& completion) :
				mCalculator(calculator),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, StreamDataResult result) override {
					if (result == StreamDataResult::kCanceled) {
						mCompletion->Call(h_, CalculateDataCRC32Result::kCanceled, 0);
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "CalculateDataCRC32: dataProvider returned an error");
						mCompletion->Call(h_, CalculateDataCRC32Result::kError, 0);
					}
					mCompletion->Call(h_, CalculateDataCRC32Result::kSuccess, mCalculator->mCRC);
				}

				//
				CalculatorPtr mCalculator;
				CalculateDataCRC32CompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void CalculateDataCRC32(const HermitPtr& h_,
								const DataProviderBlockPtr& dataProvider,
								const CalculateDataCRC32CompletionPtr& completion) {
			auto calculator = std::make_shared<Calculator>();
			auto dataCompletion = std::make_shared<Completion>(calculator, completion);
			dataProvider->ProvideData(h_, calculator, dataCompletion);
		}
		
	} // namespace encoding
} // namespace hermit
