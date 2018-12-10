//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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
#include "Hermit/String/BinaryStringToHex.h"
#include "SHA256.h"
#include "CalculateSHA256FromStream.h"

namespace hermit {
	namespace encoding {
		namespace CalculateSHA256FromStream_Impl {
			
			//
			class SHA256StreamCalculator : public DataReceiver {
			public:
				//
				SHA256StreamCalculator() {
					SHA256Init(mState);
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
					SHA256ProcessBytes(mState, data.first, data.second);
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				SHA256State mState;
			};
			typedef std::shared_ptr<SHA256StreamCalculator> SHA256StreamCalculatorPtr;

			//
			class Completion : public DataCompletion {
			public:
				//
				Completion(const SHA256StreamCalculatorPtr& calculator, const CalculateHashCompletionPtr& completion) :
				mCalculator(calculator),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const StreamDataResult& result) override {
					if (result == StreamDataResult::kCanceled) {
						mCompletion->Call(h_, CalculateHashResult::kCanceled, "");
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "CalculateSHA256FromStream: dataProvider returned an error.");
						mCompletion->Call(h_, CalculateHashResult::kError, "");
						return;
					}
					
					char buf[32] = { 0 };
					SHA256Finish(mCalculator->mState, buf);
					std::string sha256 = std::string(buf, 32);
					std::string sha256Hex;
					string::BinaryStringToHex(sha256, sha256Hex);
					mCompletion->Call(h_, CalculateHashResult::kSuccess, sha256Hex);
				}
				
				//
				SHA256StreamCalculatorPtr mCalculator;
				CalculateHashCompletionPtr mCompletion;
			};

		} // namespace CalculateSHA256FromStream_Impl
		using namespace CalculateSHA256FromStream_Impl;
		
		//
		void CalculateSHA256FromStream(const HermitPtr& h_,
									   const DataProviderPtr& dataProvider,
									   const CalculateHashCompletionPtr& completion) {
			auto calculator = std::make_shared<SHA256StreamCalculator>();
			auto dataCompletion = std::make_shared<Completion>(calculator, completion);
			dataProvider->Call(h_, calculator, dataCompletion);
		}
		
	} // namespace encoding
} // namespace hermit
