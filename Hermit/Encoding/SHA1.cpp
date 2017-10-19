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

#include <stddef.h>
#include "SHA1.h"

namespace hermit {
namespace encoding {

//
//	RFC3174 - US Secure Hash Algorithm 1 (SHA1)
//	http://www.faqs.org/rfcs/rfc3174.html
//

//
//
#ifdef E38_CONFIG_BIGENDIAN
#define E38_SWAP(n) (n)
#else
#define E38_SWAP(n) (((n) << 24) | (((n) & 0xff00) << 8) | (((n) >> 8) & 0xff00) | ((n) >> 24))
#endif

//
//
template <
	typename T>
struct Sizer
{
	char c;
	T x;
};
    
//
//
#define E38_ALIGNMENT_OF(type) offsetof(Sizer<type>, x)

//
//
#define E38_UNALIGNED_PTR(p) (((size_t) p) % E38_ALIGNMENT_OF(uint32_t) != 0)

//
//
#define E38_ROL(x, n) (((x) << (n)) | ((uint32_t) (x) >> (32 - (n))))

//
//
#define SHA1_M(I) (temp = x[I & 0x0f] ^ x[(I - 14) & 0x0f] \
		           ^ x[(I - 8) & 0x0f] ^ x[(I - 3) & 0x0f] \
	               , (x[I & 0x0f] = E38_ROL(temp, 1)))

//
//
#define SHA1_R(hash0, hash1, hash2, hash3, hash4, F, K, M) \
	do \
	{ \
		hash4 += E38_ROL(hash0, 5) \
		     + F(hash1, hash2, hash3) \
			 + K \
			 + M; \
		hash1 = E38_ROL(hash1, 30); \
	} \
	while(0)

//
//
#define SHA1_F1(hash1, hash2, hash3) (hash3 ^ (hash1 & (hash2 ^ hash3)))
#define SHA1_F2(hash1, hash2, hash3) (hash1 ^ hash2 ^ hash3)
#define SHA1_F3(hash1, hash2, hash3) ((hash1 & hash2) | (hash3 & (hash1 | hash2)))
#define SHA1_F4(hash1, hash2, hash3) (hash1 ^ hash2 ^ hash3)

//
//
#define SHA1_K1 0x5a827999
#define SHA1_K2 0x6ed9eba1
#define SHA1_K3 0x8f1bbcdc
#define SHA1_K4 0xca62c1d6

//
//
struct SHA1Context
{
	uint32_t mIntermediateHash0;
	uint32_t mIntermediateHash1;
	uint32_t mIntermediateHash2;
	uint32_t mIntermediateHash3;
	uint32_t mIntermediateHash4;
	uint32_t mTotal[2];
	uint32_t mLength;
	uint32_t mBuffer[32];
};

//
//
static const unsigned char sFillBuf[64] = { 0x80, 0 };

//
//
class SHA1EncoderImpl
{
public:
	//
	//
	SHA1EncoderImpl()
	{
		InitContext(mContext);
	}
	
	//
	//
	~SHA1EncoderImpl()
	{
	}
	
	//
	//
	void ProcessBytes(
		const char* inData,
		const size_t& inDataSize)
	{
		ProcessBytes(inData, inDataSize, mContext);
	}
	
	//
	//
	std::string GetResult()
	{
		std::string result;
		FinishContext(mContext, result);
		return result;
	}
	
private:
	//
	//
	static void InitContext(
		SHA1Context& inContext)
	{
		inContext.mIntermediateHash0 = 0x67452301;
		inContext.mIntermediateHash1 = 0xefcdab89;
		inContext.mIntermediateHash2 = 0x98badcfe;
		inContext.mIntermediateHash3 = 0x10325476;
		inContext.mIntermediateHash4 = 0xc3d2e1f0;
		inContext.mTotal[0] = inContext.mTotal[1] = 0;
		inContext.mLength = 0;
	}

	//
	//
	static void ProcessBytes(
		const void* inBuffer, 
		size_t inLen, 
		SHA1Context& inContext)
	{
		if (inContext.mLength != 0)
		{
			size_t leftOver = inContext.mLength;
			size_t add = 128 - leftOver > inLen ? inLen : 128 - leftOver;
			memcpy(&((char*)inContext.mBuffer)[leftOver], inBuffer, add);
			inContext.mLength += add;
			if (inContext.mLength > 64)
			{
				ProcessBlock(inContext.mBuffer, inContext.mLength & ~63, inContext);
				inContext.mLength &= 63;
				memcpy(
					inContext.mBuffer,
					&((char*)inContext.mBuffer)[(leftOver + add) & ~63],
					inContext.mLength);
			}
			inBuffer = (const char*)inBuffer + add;
			inLen -= add;
		}

		if (inLen >= 64)
		{
			if (E38_UNALIGNED_PTR(inBuffer))
			{
				while (inLen > 64)
				{
					ProcessBlock(memcpy(inContext.mBuffer, inBuffer, 64), 64, inContext);
					inBuffer = (const char*)inBuffer + 64;
					inLen -= 64;
				}
			}
			else
			{
				ProcessBlock(inBuffer, inLen & ~63, inContext);
				inBuffer = (const char*)inBuffer + (inLen & ~63);
				inLen &= 63;
			}
		}

		if (inLen > 0)
		{
			size_t leftOver = inContext.mLength;
			memcpy(&((char*)inContext.mBuffer)[leftOver], inBuffer, inLen);
			leftOver += inLen;
			if (leftOver >= 64)
			{
				ProcessBlock(inContext.mBuffer, 64, inContext);
				leftOver -= 64;
				memcpy(inContext.mBuffer, &inContext.mBuffer[16], leftOver);
			}
			inContext.mLength = (uint32_t)leftOver;
		}
	}

	//
	//
	static void ProcessBlock(
		const void* inBuffer, 
		size_t inLen, 
		SHA1Context& inContext)
	{
		const uint32_t* words = (const uint32_t*)inBuffer;
		size_t numWords = inLen / sizeof(uint32_t);
		const uint32_t* end = words + numWords;
		uint32_t x[16];
		uint32_t hash0 = inContext.mIntermediateHash0;
		uint32_t hash1 = inContext.mIntermediateHash1;
		uint32_t hash2 = inContext.mIntermediateHash2;
		uint32_t hash3 = inContext.mIntermediateHash3;
		uint32_t hash4 = inContext.mIntermediateHash4;

		inContext.mTotal[0] += inLen;
		if (inContext.mTotal[0] < inLen)
		{
			++inContext.mTotal[1];
		}

		while (words < end)
		{
			for (int i = 0; i < 16; ++i)
			{
				x[i] = E38_SWAP(*words);
				words++;
			}
			uint32_t temp = 0;
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F1, SHA1_K1, x[ 0]);
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F1, SHA1_K1, x[ 1]);
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F1, SHA1_K1, x[ 2]);
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F1, SHA1_K1, x[ 3]);
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F1, SHA1_K1, x[ 4]);
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F1, SHA1_K1, x[ 5]);
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F1, SHA1_K1, x[ 6]);
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F1, SHA1_K1, x[ 7]);
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F1, SHA1_K1, x[ 8]);
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F1, SHA1_K1, x[ 9]);
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F1, SHA1_K1, x[10]);
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F1, SHA1_K1, x[11]);
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F1, SHA1_K1, x[12]);
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F1, SHA1_K1, x[13]);
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F1, SHA1_K1, x[14]);
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F1, SHA1_K1, x[15]);
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F1, SHA1_K1, SHA1_M(16));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F1, SHA1_K1, SHA1_M(17));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F1, SHA1_K1, SHA1_M(18));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F1, SHA1_K1, SHA1_M(19));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F2, SHA1_K2, SHA1_M(20));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F2, SHA1_K2, SHA1_M(21));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F2, SHA1_K2, SHA1_M(22));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F2, SHA1_K2, SHA1_M(23));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F2, SHA1_K2, SHA1_M(24));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F2, SHA1_K2, SHA1_M(25));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F2, SHA1_K2, SHA1_M(26));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F2, SHA1_K2, SHA1_M(27));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F2, SHA1_K2, SHA1_M(28));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F2, SHA1_K2, SHA1_M(29));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F2, SHA1_K2, SHA1_M(30));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F2, SHA1_K2, SHA1_M(31));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F2, SHA1_K2, SHA1_M(32));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F2, SHA1_K2, SHA1_M(33));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F2, SHA1_K2, SHA1_M(34));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F2, SHA1_K2, SHA1_M(35));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F2, SHA1_K2, SHA1_M(36));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F2, SHA1_K2, SHA1_M(37));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F2, SHA1_K2, SHA1_M(38));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F2, SHA1_K2, SHA1_M(39));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F3, SHA1_K3, SHA1_M(40));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F3, SHA1_K3, SHA1_M(41));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F3, SHA1_K3, SHA1_M(42));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F3, SHA1_K3, SHA1_M(43));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F3, SHA1_K3, SHA1_M(44));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F3, SHA1_K3, SHA1_M(45));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F3, SHA1_K3, SHA1_M(46));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F3, SHA1_K3, SHA1_M(47));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F3, SHA1_K3, SHA1_M(48));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F3, SHA1_K3, SHA1_M(49));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F3, SHA1_K3, SHA1_M(50));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F3, SHA1_K3, SHA1_M(51));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F3, SHA1_K3, SHA1_M(52));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F3, SHA1_K3, SHA1_M(53));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F3, SHA1_K3, SHA1_M(54));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F3, SHA1_K3, SHA1_M(55));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F3, SHA1_K3, SHA1_M(56));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F3, SHA1_K3, SHA1_M(57));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F3, SHA1_K3, SHA1_M(58));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F3, SHA1_K3, SHA1_M(59));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F4, SHA1_K4, SHA1_M(60));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F4, SHA1_K4, SHA1_M(61));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F4, SHA1_K4, SHA1_M(62));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F4, SHA1_K4, SHA1_M(63));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F4, SHA1_K4, SHA1_M(64));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F4, SHA1_K4, SHA1_M(65));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F4, SHA1_K4, SHA1_M(66));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F4, SHA1_K4, SHA1_M(67));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F4, SHA1_K4, SHA1_M(68));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F4, SHA1_K4, SHA1_M(69));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F4, SHA1_K4, SHA1_M(70));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F4, SHA1_K4, SHA1_M(71));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F4, SHA1_K4, SHA1_M(72));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F4, SHA1_K4, SHA1_M(73));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F4, SHA1_K4, SHA1_M(74));
			SHA1_R(hash0, hash1, hash2, hash3, hash4, SHA1_F4, SHA1_K4, SHA1_M(75));
			SHA1_R(hash4, hash0, hash1, hash2, hash3, SHA1_F4, SHA1_K4, SHA1_M(76));
			SHA1_R(hash3, hash4, hash0, hash1, hash2, SHA1_F4, SHA1_K4, SHA1_M(77));
			SHA1_R(hash2, hash3, hash4, hash0, hash1, SHA1_F4, SHA1_K4, SHA1_M(78));
			SHA1_R(hash1, hash2, hash3, hash4, hash0, SHA1_F4, SHA1_K4, SHA1_M(79));

			hash0 = inContext.mIntermediateHash0 += hash0;
			hash1 = inContext.mIntermediateHash1 += hash1;
			hash2 = inContext.mIntermediateHash2 += hash2;
			hash3 = inContext.mIntermediateHash3 += hash3;
			hash4 = inContext.mIntermediateHash4 += hash4;
		}
	}

	//
	//
	static void FinishContext(
		SHA1Context& inContext, 
		std::string& outResult)
	{
		uint32_t bytes = inContext.mLength;
		size_t size = (bytes < 56) ? 64 / 4 : 64 * 2 / 4;
		inContext.mTotal[0] += bytes;
		if (inContext.mTotal[0] < bytes)
		{
			++inContext.mTotal[1];
		}
		inContext.mBuffer[size - 2] = E38_SWAP((inContext.mTotal[1] << 3) | (inContext.mTotal[0] >> 29));
		inContext.mBuffer[size - 1] = E38_SWAP(inContext.mTotal[0] << 3);
		memcpy(&((char*) inContext.mBuffer)[bytes], sFillBuf, (size - 2) * 4 - bytes);
		ProcessBlock(inContext.mBuffer, size * 4, inContext);
		ReadContext(inContext, outResult);
	}

	//
	//
	static void ReadContext(
		const SHA1Context& inContext, 
		std::string& outResult)
	{
		char result[20];
		((uint32_t*) result)[0] = E38_SWAP(inContext.mIntermediateHash0);
		((uint32_t*) result)[1] = E38_SWAP(inContext.mIntermediateHash1);
		((uint32_t*) result)[2] = E38_SWAP(inContext.mIntermediateHash2);
		((uint32_t*) result)[3] = E38_SWAP(inContext.mIntermediateHash3);
		((uint32_t*) result)[4] = E38_SWAP(inContext.mIntermediateHash4);
		outResult = std::string(result, sizeof(result));
	}

	//
	//
	SHA1Context mContext;
};

//
//
SHA1Encoder::SHA1Encoder()
	:
	mImpl(new SHA1EncoderImpl())
{
}
	
//
//
SHA1Encoder::~SHA1Encoder()
{
	delete mImpl;
}
	
//
//
void SHA1Encoder::ProcessBytes(
	const char* inData,
	const size_t& inDataSize)
{
	mImpl->ProcessBytes(inData, inDataSize);
}

//
//
std::string SHA1Encoder::GetResult()
{
	return mImpl->GetResult();
}

} // namespace encoding
} // namespace hermit
