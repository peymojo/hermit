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

#include <string>
#include "Hermit/Foundation/Notification.h"
#include "CalculateMurmur3_128.h"

namespace hermit {
	namespace encoding {
		
		namespace {
			
			//-----------------------------------------------------------------------------
			// MurmurHash3 was written by Austin Appleby, and is placed in the public
			// domain. The author hereby disclaims copyright to this source code.
			
			// Note - The x86 and x64 versions do _not_ produce the same results, as the
			// algorithms are optimized for their respective platforms. You can still
			// compile and run any of them on any platform, but your performance with the
			// non-native version will be less than optimal.
			
#define	FORCE_INLINE inline __attribute__((always_inline))
			
			inline uint64_t rotl64 ( uint64_t x, int8_t r ) {
				return (x << r) | (x >> (64 - r));
			}
			
#define ROTL64(x,y)	rotl64(x,y)
			
#define BIG_CONSTANT(x) (x##LLU)
			
			//-----------------------------------------------------------------------------
			// Finalization mix - force all bits of a hash block to avalanche
			
			FORCE_INLINE uint64_t fmix64 ( uint64_t k ) {
				k ^= k >> 33;
				k *= BIG_CONSTANT(0xff51afd7ed558ccd);
				k ^= k >> 33;
				k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
				k ^= k >> 33;
				
				return k;
			}
			
			//
			class MurmurCalculator : public DataHandlerBlock {
				//
				const uint64_t kBlockSize = 16;
				const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
				const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);
				
			public:
				//
				MurmurCalculator(int32_t inSeed) : h1(inSeed), h2(inSeed), mTotalBytes(0) {
					mResult[0] = 0;
					mResult[1] = 0;
				}
				
				//
				virtual StreamDataResult HandleData(const HermitPtr& h_, const DataBuffer& data, bool isEndOfData) override {
					mTotalBytes += data.second;
					
					size_t bytesRemaining = (size_t)data.second;
					size_t offset = 0;
					if (!mCarryoverBuffer.empty()) {
						size_t bytesNeededToComplete1Block(kBlockSize - mCarryoverBuffer.size());
						size_t bytesToCopy = std::min(bytesNeededToComplete1Block, bytesRemaining);
						mCarryoverBuffer.append(data.first, bytesToCopy);
						bytesRemaining -= bytesToCopy;
						offset += bytesToCopy;
						
						if (mCarryoverBuffer.size() == kBlockSize) {
							uint64_t bytesConsumed = 0;
							ProcessBlocks(mCarryoverBuffer.data(), mCarryoverBuffer.size(), bytesConsumed);
							mCarryoverBuffer.clear();
						}
					}
					
					uint64_t bytesConsumed = 0;
					ProcessBlocks(data.first + offset, bytesRemaining, bytesConsumed);
					
					bytesRemaining -= bytesConsumed;
					if (bytesRemaining > 0) {
						mCarryoverBuffer = std::string(data.first + offset + bytesConsumed, bytesRemaining);
					}
					
					if (isEndOfData) {
						//----------
						// tail
						const uint8_t * tail = (const uint8_t*)data.first + offset + bytesConsumed;
						
						uint64_t k1 = 0;
						uint64_t k2 = 0;
						
						switch(mTotalBytes & 15) {
							case 15: k2 ^= ((uint64_t)tail[14]) << 48;
							case 14: k2 ^= ((uint64_t)tail[13]) << 40;
							case 13: k2 ^= ((uint64_t)tail[12]) << 32;
							case 12: k2 ^= ((uint64_t)tail[11]) << 24;
							case 11: k2 ^= ((uint64_t)tail[10]) << 16;
							case 10: k2 ^= ((uint64_t)tail[ 9]) << 8;
							case  9: k2 ^= ((uint64_t)tail[ 8]) << 0;
								k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
								
							case  8: k1 ^= ((uint64_t)tail[ 7]) << 56;
							case  7: k1 ^= ((uint64_t)tail[ 6]) << 48;
							case  6: k1 ^= ((uint64_t)tail[ 5]) << 40;
							case  5: k1 ^= ((uint64_t)tail[ 4]) << 32;
							case  4: k1 ^= ((uint64_t)tail[ 3]) << 24;
							case  3: k1 ^= ((uint64_t)tail[ 2]) << 16;
							case  2: k1 ^= ((uint64_t)tail[ 1]) << 8;
							case  1: k1 ^= ((uint64_t)tail[ 0]) << 0;
								k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
						};
						
						//----------
						// finalization
						
						h1 ^= mTotalBytes;
						h2 ^= mTotalBytes;
						
						h1 += h2;
						h2 += h1;
						
						h1 = fmix64(h1);
						h2 = fmix64(h2);
						
						h1 += h2;
						h2 += h1;
						
						mResult[0] = h1;
						mResult[1] = h2;
					}
					return StreamDataResult::kSuccess;
				}
				
				//
				void ProcessBlocks(const char* inData, uint64_t inDataSize, uint64_t& outBytesConsumed) {
					std::string::size_type size = (std::string::size_type)inDataSize;
					std::string::size_type offset = 0;
					while (size >= kBlockSize) {
						uint64_t k1 = *(uint64_t*)(inData + offset);
						uint64_t k2 = *((uint64_t*)(inData + offset) + 1);
						
						k1 *= c1; k1  = ROTL64(k1,31); k1 *= c2; h1 ^= k1;
						
						h1 = ROTL64(h1,27); h1 += h2; h1 = h1*5+0x52dce729;
						
						k2 *= c2; k2  = ROTL64(k2,33); k2 *= c1; h2 ^= k2;
						
						h2 = ROTL64(h2,31); h2 += h1; h2 = h2*5+0x38495ab5;
						
						offset += kBlockSize;
						size -= kBlockSize;
					}
					outBytesConsumed = offset;
				}
				
				//
				uint64_t h1;
				uint64_t h2;
				uint64_t mTotalBytes;
				std::string mCarryoverBuffer;
				uint64_t mResult[2];
			};
			
			//
			void MurmurResultToHex(const char* inResult, std::string& outResult) {
				char buf[256];
				
				sprintf(buf,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						inResult[0] & 0xff,
						inResult[1] & 0xff,
						inResult[2] & 0xff,
						inResult[3] & 0xff,
						inResult[4] & 0xff,
						inResult[5] & 0xff,
						inResult[6] & 0xff,
						inResult[7] & 0xff,
						inResult[8] & 0xff,
						inResult[9] & 0xff,
						inResult[10] & 0xff,
						inResult[11] & 0xff,
						inResult[12] & 0xff,
						inResult[13] & 0xff,
						inResult[14] & 0xff,
						inResult[15] & 0xff);
				
				outResult = buf;
			}
			typedef std::shared_ptr<MurmurCalculator> MurmurCalculatorPtr;
			
			//
			class Completion : public StreamResultBlock {
			public:
				//
				Completion(const MurmurCalculatorPtr& calculator, const CalculateHashCompletionPtr& completion) :
				mCalculator(calculator),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, StreamDataResult result) override {
					if (result == StreamDataResult::kCanceled) {
						mCompletion->Call(h_, CalculateHashResult::kCanceled, "");
						return;
					}
					if (result != StreamDataResult::kSuccess) {
						NOTIFY_ERROR(h_, "CalculateMurmur3_128: dataProvider failed.");
						mCompletion->Call(h_, CalculateHashResult::kError, "");
						return;
					}
					std::string murmurHex;
					MurmurResultToHex((const char*)mCalculator->mResult, murmurHex);
					mCompletion->Call(h_, CalculateHashResult::kSuccess, murmurHex);
				}
				
				//
				MurmurCalculatorPtr mCalculator;
				CalculateHashCompletionPtr mCompletion;
			};
			
		} // private namespace
		
		//
		void CalculateMurmur3_128(const HermitPtr& h_,
								  const DataProviderBlockPtr& dataProvider,
								  const CalculateHashCompletionPtr& completion) {
			auto calculator = std::make_shared<MurmurCalculator>(0);
			auto dataCompletion = std::make_shared<Completion>(calculator, completion);
			dataProvider->ProvideData(h_, calculator, dataCompletion);
		}
		
	} // namespace encoding
} // namespace hermit
