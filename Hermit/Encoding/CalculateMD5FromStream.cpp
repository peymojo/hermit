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

#include <math.h>
#include "Hermit/Foundation/Notification.h"
#include "CalculateMD5FromStream.h"

namespace hermit {
	namespace encoding {
		namespace CalculateMD5FromStream_Impl {
			
			//
			struct MD5Result {
				uint32_t m0;
				uint32_t m1;
				uint32_t m2;
				uint32_t m3;
			};
			
			//
			static const uint32_t sR[64] = {
				7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,  7, 12, 17, 22,
				5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,  5,  9, 14, 20,
				4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,  4, 11, 16, 23,
				6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21,  6, 10, 15, 21 };
			
			//
			static const uint32_t sK[64] = {
				3614090360ul,
				3905402710ul,
				606105819ul,
				3250441966ul,
				4118548399ul,
				1200080426ul,
				2821735955ul,
				4249261313ul,
				1770035416ul,
				2336552879ul,
				4294925233ul,
				2304563134ul,
				1804603682ul,
				4254626195ul,
				2792965006ul,
				1236535329ul,
				4129170786ul,
				3225465664ul,
				643717713ul,
				3921069994ul,
				3593408605ul,
				38016083ul,
				3634488961ul,
				3889429448ul,
				568446438ul,
				3275163606ul,
				4107603335ul,
				1163531501ul,
				2850285829ul,
				4243563512ul,
				1735328473ul,
				2368359562ul,
				4294588738ul,
				2272392833ul,
				1839030562ul,
				4259657740ul,
				2763975236ul,
				1272893353ul,
				4139469664ul,
				3200236656ul,
				681279174ul,
				3936430074ul,
				3572445317ul,
				76029189ul,
				3654602809ul,
				3873151461ul,
				530742520ul,
				3299628645ul,
				4096336452ul,
				1126891415ul,
				2878612391ul,
				4237533241ul,
				1700485571ul,
				2399980690ul,
				4293915773ul,
				2240044497ul,
				1873313359ul,
				4264355552ul,
				2734768916ul,
				1309151649ul,
				4149444226ul,
				3174756917ul,
				718787259ul,
				3951481745ul  };
			
			//
			uint32_t RotateLeft(uint32_t inX, uint32_t inC) {
				return (inX << inC) | ((inX >> (32 - inC)));
			}
			
			//
			class MD5StreamCalculator : public DataReceiver {
				//
				static const uint64_t kBlockSize = 64;
				
			public:
				//
				MD5StreamCalculator() : mTotalBytes(0) {
					mResult.m0 = 0x67452301;
					mResult.m1 = 0xEFCDAB89;
					mResult.m2 = 0x98BADCFE;
					mResult.m3 = 0x10325476;
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const DataBuffer& data,
								  const bool& isEndOfData,
								  const DataCompletionPtr& completion) override {
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
					
					std::string trailingBytes;
					if (isEndOfData) {
						uint64_t inputStringLengthInBits = mTotalBytes * 8;
						uint64_t totalBits = inputStringLengthInBits + 65;
						uint64_t remainderBits = totalBits % 512;
						uint64_t paddingBits = 512 - remainderBits;
						uint64_t paddingBytes = (paddingBits / 8) + 1;
						
						trailingBytes.push_back(0x80);
						--paddingBytes;
						for (int i = 0; i < paddingBytes; ++i) {
							trailingBytes.push_back(0);
						}
						
						char stringLength0 = inputStringLengthInBits & 0xff;
						char stringLength1 = (inputStringLengthInBits >> 8) & 0xff;
						char stringLength2 = (inputStringLengthInBits >> 16) & 0xff;
						char stringLength3 = (inputStringLengthInBits >> 24) & 0xff;
						char stringLength4 = (inputStringLengthInBits >> 32) & 0xff;
						char stringLength5 = (inputStringLengthInBits >> 40) & 0xff;
						char stringLength6 = (inputStringLengthInBits >> 48) & 0xff;
						char stringLength7 = (inputStringLengthInBits >> 56) & 0xff;
						
						trailingBytes.push_back(stringLength0);
						trailingBytes.push_back(stringLength1);
						trailingBytes.push_back(stringLength2);
						trailingBytes.push_back(stringLength3);
						trailingBytes.push_back(stringLength4);
						trailingBytes.push_back(stringLength5);
						trailingBytes.push_back(stringLength6);
						trailingBytes.push_back(stringLength7);
					}
					if ((mCarryoverBuffer.size() < kBlockSize) && !trailingBytes.empty()) {
						mCarryoverBuffer += trailingBytes;
					}
					if (mCarryoverBuffer.size() >= kBlockSize) {
						uint64_t bytesConsumed = 0;
						ProcessBlocks(mCarryoverBuffer.data(), mCarryoverBuffer.size(), bytesConsumed);
						mCarryoverBuffer = mCarryoverBuffer.substr((size_t)bytesConsumed);
					}
					completion->Call(h_, StreamDataResult::kSuccess);
				}
				
				//
				void ProcessBlocks(const char* inData, uint64_t inDataSize, uint64_t& outBytesConsumed) {
					std::string::size_type size = (std::string::size_type)inDataSize;
					std::string::size_type offset = 0;
					while (size >= kBlockSize) {
						ProcessBlock(inData + offset, mResult);
						offset += kBlockSize;
						size -= kBlockSize;
					}
					outBytesConsumed = offset;
				}
				
				//
				static void ProcessBlock(const char* inBlock, MD5Result& ioResult) {
					uint32_t w[16];
					int offset = 0;
					for (int j = 0; j < 16; ++j) {
						uint32_t byte0 = inBlock[offset++] & 0xff;
						uint32_t byte1 = inBlock[offset++] & 0xff;
						uint32_t byte2 = inBlock[offset++] & 0xff;
						uint32_t byte3 = inBlock[offset++] & 0xff;
						w[j] = (byte3 << 24) | (byte2 << 16) | (byte1 << 8) | byte0;
					}
					
					uint32_t a = ioResult.m0;
					uint32_t b = ioResult.m1;
					uint32_t c = ioResult.m2;
					uint32_t d = ioResult.m3;
					
					//			printf("-> a: %08x, b: %08x, c: %08x, d: %08x\n", a, b, c, d);
					
					uint32_t f = 0;
					uint32_t g = 0;
					for (int i = 0; i < 64; ++i) {
						if (i < 16) {
							f = (b & c) | ((~b) & d);
							g = i;
						}
						else if (i < 32) {
							f = (d & b) | ((~d) & c);
							g = ((5 * i) + 1) % 16;
						}
						else if (i < 48) {
							f = b ^ c ^ d;
							g = ((3 * i) + 5) % 16;
						}
						else {
							f = c ^ (b | (~d));
							g = (7 * i) % 16;
						}
						
						//				printf("zzz: %08x\n", f);
						//				printf("kk: %08x\n", sK[i]);
						//				printf("w[g]: %08x\n", w[g]);
						//				uint32_t qqq = (a + f + sK[i] + w[g]);
						//				printf("qqq: %08x\n", qqq);
						
						uint32_t tmp = d;
						d = c;
						c = b;
						b = b + RotateLeft((a + f + sK[i] + w[g]), sR[i]);
						a = tmp;
						
						//				printf("-[%02d]- a: %08x, b: %08x, c: %08x, d: %08x\n", i, a, b, c, d);
					}
					
					//			printf("<- a: %08x, b: %08x, c: %08x, d: %08x\n", a, b, c, d);
					
					ioResult.m0 += a;
					ioResult.m1 += b;
					ioResult.m2 += c;
					ioResult.m3 += d;
				}
				
				//
				MD5Result mResult;
				uint64_t mTotalBytes;
				std::string mCarryoverBuffer;
			};
			typedef std::shared_ptr<MD5StreamCalculator> MD5StreamCalculatorPtr;
			
			//
			std::string MD5ResultToString(const MD5Result& inMD5Result) {
				char buf[256];
				sprintf(buf,
						"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
						inMD5Result.m0 & 0xff,
						(inMD5Result.m0 >> 8) & 0xff,
						(inMD5Result.m0 >> 16) & 0xff,
						(inMD5Result.m0 >> 24) & 0xff,
						inMD5Result.m1 & 0xff,
						(inMD5Result.m1 >> 8) & 0xff,
						(inMD5Result.m1 >> 16) & 0xff,
						(inMD5Result.m1 >> 24) & 0xff,
						inMD5Result.m2 & 0xff,
						(inMD5Result.m2 >> 8) & 0xff,
						(inMD5Result.m2 >> 16) & 0xff,
						(inMD5Result.m2 >> 24) & 0xff,
						inMD5Result.m3 & 0xff,
						(inMD5Result.m3 >> 8) & 0xff,
						(inMD5Result.m3 >> 16) & 0xff,
						(inMD5Result.m3 >> 24) & 0xff);
				
				return std::string(buf);
			}
			
			//
			class Completion : public DataCompletion {
			public:
				//
				Completion(const MD5StreamCalculatorPtr& calculator, const CalculateHashCompletionPtr& completion) :
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
						NOTIFY_ERROR(h_, "CalculateMD5FromStream: dataProvider returned an error.");
						mCompletion->Call(h_, CalculateHashResult::kError, "");
						return;
					}
					std::string md5Hex = MD5ResultToString(mCalculator->mResult);
					mCompletion->Call(h_, CalculateHashResult::kSuccess, md5Hex);
				}
				
				//
				MD5StreamCalculatorPtr mCalculator;
				CalculateHashCompletionPtr mCompletion;
			};
			
		} // namespace CalculateMD5FromStream_Impl
		using namespace CalculateMD5FromStream_Impl;
		
		//
		void CalculateMD5FromStream(const HermitPtr& h_,
									const DataProviderPtr& dataProvider,
									const CalculateHashCompletionPtr& completion) {
			auto calculator = std::make_shared<MD5StreamCalculator>();
			auto dataCompletion = std::make_shared<Completion>(calculator, completion);
			dataProvider->Call(h_, calculator, dataCompletion);
		}
		
	} // namespace encoding
} // namespace hermit
