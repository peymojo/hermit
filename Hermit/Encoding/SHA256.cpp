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
#include <string.h>
#include "SHA256.h"

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
#define E38_alignof(type) offsetof(Sizer<type>, x)
#define E38_UNALIGNED_P(p) (((uint64_t) p) % E38_alignof(uint32_t) != 0)

namespace hermit {
namespace encoding {

namespace
{

	//	This array contains the bytes used to pad the buffer to the next
	//	64-byte boundary.
	static const unsigned char sFillBuf[64] = { 0x80, 0 /* , 0, 0, ...  */ };

	//	Round functions. 
	#define E38_SHA256_F2(A,B,C) ( ( A & B ) | ( C & ( A | B ) ) )
	#define E38_SHA256_F1(E,F,G) ( G ^ ( E & ( F ^ G ) ) )

	//	SHA256 round constants
	#define E38_SHA256_K(I) SHA256_round_constants[I]
	static const uint32_t SHA256_round_constants[64] = {
	  0x428a2f98UL, 0x71374491UL, 0xb5c0fbcfUL, 0xe9b5dba5UL,
	  0x3956c25bUL, 0x59f111f1UL, 0x923f82a4UL, 0xab1c5ed5UL,
	  0xd807aa98UL, 0x12835b01UL, 0x243185beUL, 0x550c7dc3UL,
	  0x72be5d74UL, 0x80deb1feUL, 0x9bdc06a7UL, 0xc19bf174UL,
	  0xe49b69c1UL, 0xefbe4786UL, 0x0fc19dc6UL, 0x240ca1ccUL,
	  0x2de92c6fUL, 0x4a7484aaUL, 0x5cb0a9dcUL, 0x76f988daUL,
	  0x983e5152UL, 0xa831c66dUL, 0xb00327c8UL, 0xbf597fc7UL,
	  0xc6e00bf3UL, 0xd5a79147UL, 0x06ca6351UL, 0x14292967UL,
	  0x27b70a85UL, 0x2e1b2138UL, 0x4d2c6dfcUL, 0x53380d13UL,
	  0x650a7354UL, 0x766a0abbUL, 0x81c2c92eUL, 0x92722c85UL,
	  0xa2bfe8a1UL, 0xa81a664bUL, 0xc24b8b70UL, 0xc76c51a3UL,
	  0xd192e819UL, 0xd6990624UL, 0xf40e3585UL, 0x106aa070UL,
	  0x19a4c116UL, 0x1e376c08UL, 0x2748774cUL, 0x34b0bcb5UL,
	  0x391c0cb3UL, 0x4ed8aa4aUL, 0x5b9cca4fUL, 0x682e6ff3UL,
	  0x748f82eeUL, 0x78a5636fUL, 0x84c87814UL, 0x8cc70208UL,
	  0x90befffaUL, 0xa4506cebUL, 0xbef9a3f7UL, 0xc67178f2UL,
	};

	//	Process LEN bytes of BUFFER, accumulating context into CTX.
	//	It is assumed that LEN % 64 == 0.
	void SHA256ProcessBlock(
		SHA256State& ioState,
		const void* inBuffer, 
		uint64_t inDataSize)
	{
		const uint32_t *words = static_cast<const uint32_t*>(inBuffer);
		uint64_t nwords = inDataSize / sizeof (uint32_t);
		const uint32_t *endp = words + nwords;
		uint32_t x[16];
		uint32_t a = ioState.state[0];
		uint32_t b = ioState.state[1];
		uint32_t c = ioState.state[2];
		uint32_t d = ioState.state[3];
		uint32_t e = ioState.state[4];
		uint32_t f = ioState.state[5];
		uint32_t g = ioState.state[6];
		uint32_t h = ioState.state[7];
		
		//	First increment the byte count.  FIPS PUB 180-2 specifies the possible
		//	length of the file up to 2^64 bits.  Here we only compute the
		//	number of bytes.  Do a double word increment.
		ioState.total[0] += inDataSize;
		if (ioState.total[0] < inDataSize)
		{
			++ioState.total[1];
		}

	#define E38_SHA256_rol(x, n) (((x) << (n)) | ((x) >> (32 - (n))))
	#define E38_SHA256_S0(x) (E38_SHA256_rol(x, 25) ^ E38_SHA256_rol(x, 14) ^ (x>>3))
	#define E38_SHA256_S1(x) (E38_SHA256_rol(x, 15) ^ E38_SHA256_rol(x, 13) ^ (x>>10))
	#define E38_SHA256_SS0(x) (E38_SHA256_rol(x, 30) ^ E38_SHA256_rol(x, 19) ^ E38_SHA256_rol(x, 10))
	#define E38_SHA256_SS1(x) (E38_SHA256_rol(x, 26) ^ E38_SHA256_rol(x, 21) ^ E38_SHA256_rol(x, 7))

	#define E38_SHA256_M(I) ( tm =   E38_SHA256_S1(x[(I - 2) & 0x0f]) + x[(I - 7) & 0x0f] \
								   + E38_SHA256_S0(x[(I - 15) & 0x0f]) + x[I & 0x0f] \
							  , x[I & 0x0f] = tm )

	#define E38_SHA256_R(A,B,C,D,E,F,G,H,K,M)	do \
												{ \
													t0 = E38_SHA256_SS0(A) + E38_SHA256_F2(A, B, C); \
													t1 = H + E38_SHA256_SS1(E) + E38_SHA256_F1(E, F, G) + K + M; \
													D += t1; \
													H = t0 + t1; \
												} while(0)

		while (words < endp)
		{
			uint32_t tm;
			uint32_t t0, t1;
			for (int t = 0; t < 16; t++)
			{
				x[t] = E38_SWAP(*words);
				words++;
			}

			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K( 0), x[ 0] );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K( 1), x[ 1] );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K( 2), x[ 2] );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K( 3), x[ 3] );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K( 4), x[ 4] );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K( 5), x[ 5] );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K( 6), x[ 6] );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K( 7), x[ 7] );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K( 8), x[ 8] );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K( 9), x[ 9] );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(10), x[10] );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(11), x[11] );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(12), x[12] );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(13), x[13] );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(14), x[14] );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(15), x[15] );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(16), E38_SHA256_M(16) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(17), E38_SHA256_M(17) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(18), E38_SHA256_M(18) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(19), E38_SHA256_M(19) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(20), E38_SHA256_M(20) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(21), E38_SHA256_M(21) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(22), E38_SHA256_M(22) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(23), E38_SHA256_M(23) );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(24), E38_SHA256_M(24) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(25), E38_SHA256_M(25) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(26), E38_SHA256_M(26) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(27), E38_SHA256_M(27) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(28), E38_SHA256_M(28) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(29), E38_SHA256_M(29) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(30), E38_SHA256_M(30) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(31), E38_SHA256_M(31) );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(32), E38_SHA256_M(32) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(33), E38_SHA256_M(33) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(34), E38_SHA256_M(34) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(35), E38_SHA256_M(35) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(36), E38_SHA256_M(36) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(37), E38_SHA256_M(37) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(38), E38_SHA256_M(38) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(39), E38_SHA256_M(39) );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(40), E38_SHA256_M(40) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(41), E38_SHA256_M(41) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(42), E38_SHA256_M(42) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(43), E38_SHA256_M(43) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(44), E38_SHA256_M(44) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(45), E38_SHA256_M(45) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(46), E38_SHA256_M(46) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(47), E38_SHA256_M(47) );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(48), E38_SHA256_M(48) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(49), E38_SHA256_M(49) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(50), E38_SHA256_M(50) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(51), E38_SHA256_M(51) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(52), E38_SHA256_M(52) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(53), E38_SHA256_M(53) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(54), E38_SHA256_M(54) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(55), E38_SHA256_M(55) );
			E38_SHA256_R( a, b, c, d, e, f, g, h, E38_SHA256_K(56), E38_SHA256_M(56) );
			E38_SHA256_R( h, a, b, c, d, e, f, g, E38_SHA256_K(57), E38_SHA256_M(57) );
			E38_SHA256_R( g, h, a, b, c, d, e, f, E38_SHA256_K(58), E38_SHA256_M(58) );
			E38_SHA256_R( f, g, h, a, b, c, d, e, E38_SHA256_K(59), E38_SHA256_M(59) );
			E38_SHA256_R( e, f, g, h, a, b, c, d, E38_SHA256_K(60), E38_SHA256_M(60) );
			E38_SHA256_R( d, e, f, g, h, a, b, c, E38_SHA256_K(61), E38_SHA256_M(61) );
			E38_SHA256_R( c, d, e, f, g, h, a, b, E38_SHA256_K(62), E38_SHA256_M(62) );
			E38_SHA256_R( b, c, d, e, f, g, h, a, E38_SHA256_K(63), E38_SHA256_M(63) );

			a = ioState.state[0] += a;
			b = ioState.state[1] += b;
			c = ioState.state[2] += c;
			d = ioState.state[3] += d;
			e = ioState.state[4] += e;
			f = ioState.state[5] += f;
			g = ioState.state[6] += g;
			h = ioState.state[7] += h;
		}
	}

} // private namespace

//
//
void SHA256Init(
	SHA256State& ioState)
{
	ioState.state[0] = 0x6a09e667UL;
	ioState.state[1] = 0xbb67ae85UL;
	ioState.state[2] = 0x3c6ef372UL;
	ioState.state[3] = 0xa54ff53aUL;
	ioState.state[4] = 0x510e527fUL;
	ioState.state[5] = 0x9b05688cUL;
	ioState.state[6] = 0x1f83d9abUL;
	ioState.state[7] = 0x5be0cd19UL;
	ioState.total[0] = ioState.total[1] = 0;
	ioState.buflen = 0;
}

//
//
void SHA224Init(
	SHA256State& ioState)
{
	ioState.state[0] = 0xc1059ed8UL;
	ioState.state[1] = 0x367cd507UL;
	ioState.state[2] = 0x3070dd17UL;
	ioState.state[3] = 0xf70e5939UL;
	ioState.state[4] = 0xffc00b31UL;
	ioState.state[5] = 0x68581511UL;
	ioState.state[6] = 0x64f98fa7UL;
	ioState.state[7] = 0xbefa4fa4UL;
	ioState.total[0] = ioState.total[1] = 0;
	ioState.buflen = 0;
}

//
//
void SHA256ProcessBytes(
	SHA256State& ioState,
	const void* inData,
	uint64_t inDataSize)
{
	const void* buffer = inData;
	uint64_t len = inDataSize;
	
	//	When we already have some bits in our internal buffer concatenate
	//	both inputs first.
	if (ioState.buflen != 0)
	{
		uint64_t left_over = ioState.buflen;
		uint64_t add = 128 - left_over > len ? len : 128 - left_over;

		memcpy(&((char *) ioState.buffer)[left_over], buffer, add);
		ioState.buflen += add;
	
		if (ioState.buflen > 64)
		{
			SHA256ProcessBlock(ioState, ioState.buffer, ioState.buflen & ~63);

			ioState.buflen &= 63;
			//	The regions in the following copy operation cannot overlap.
			memcpy(
				ioState.buffer,
				&((char *) ioState.buffer)[(left_over + add) & ~63],
				ioState.buflen);
		}

		buffer = (const char *) buffer + add;
		len -= add;
	}

	//	Process available complete blocks.
	if (len >= 64)
	{
		if (E38_UNALIGNED_P(buffer))
		{
			while (len > 64)
			{
				SHA256ProcessBlock(ioState, memcpy(ioState.buffer, buffer, 64), 64);
				buffer = (const char *) buffer + 64;
				len -= 64;
			}
		}
		else
		{
			SHA256ProcessBlock(ioState, buffer, len & ~63);
			buffer = (const char *) buffer + (len & ~63);
			len &= 63;
		}
	}

	//	Move remaining bytes in internal buffer.
	if (len > 0)
	{
		uint32_t left_over = ioState.buflen;

		memcpy(&((char *) ioState.buffer)[left_over], buffer, len);
		left_over += len;
		if (left_over >= 64)
		{
			SHA256ProcessBlock(ioState, ioState.buffer, 64);
			left_over -= 64;
			memcpy(ioState.buffer, &ioState.buffer[16], left_over);
		}
		ioState.buflen = left_over;
	}
}

//
//
void* SHA256Read(
	const SHA256State& inState,
	void* outResult)
{
	for (int i = 0; i < 8; i++)
	{
		((uint32_t *) outResult)[i] = E38_SWAP(inState.state[i]);
	}

	return outResult;
}

//
//
void* SHA224Read(
	const SHA256State& inState, 
	void* outResult)
{
	for (int i = 0; i < 7; i++)
	{
		((uint32_t *) outResult)[i] = E38_SWAP(inState.state[i]);
	}
	return outResult;
}

//
//
static void SHA256Conclude(
	SHA256State& ioState)
{
	//	Take yet unprocessed bytes into account.
	uint32_t bytes = ioState.buflen;
	uint64_t size = (bytes < 56) ? 64 / 4 : 64 * 2 / 4;

	//	Now count remaining bytes.
	ioState.total[0] += bytes;
	if (ioState.total[0] < bytes)
	{
		++ioState.total[1];
	}

	//	Put the 64-bit file length in *bits* at the end of the buffer.
	ioState.buffer[size - 2] = E38_SWAP((ioState.total[1] << 3) | (ioState.total[0] >> 29));
	ioState.buffer[size - 1] = E38_SWAP(ioState.total[0] << 3);

	memcpy(&((char *) ioState.buffer)[bytes], sFillBuf, (size - 2) * 4 - bytes);

	//	Process last bytes.
	SHA256ProcessBlock(ioState, ioState.buffer, size * 4);
}

//
//
void* SHA256Finish(
	SHA256State& ioState, 
	void* outResult)
{
	SHA256Conclude(ioState);
	return SHA256Read(ioState, outResult);
}

//
//
void* SHA224Finish(
	SHA256State& ioState,
	void* outResult)
{
	SHA256Conclude(ioState);
	return SHA224Read(ioState, outResult);
}

//
//
void* CalculateSHA256(
	const char* inData, 
	uint64_t inDataSize, 
	void* outResult)
{
	SHA256State state;
	SHA256Init(state);
	SHA256ProcessBytes(state, inData, inDataSize);
	return SHA256Finish(state, outResult);
}

//
//
void* CalculateSHA224(
	const char* inData, 
	uint64_t inDataSize, 
	void* outResult)
{
	SHA256State state;
	SHA224Init(state);
	SHA256ProcessBytes(state, inData, inDataSize);
	return SHA224Finish(state, outResult);
}

} // namespace encoding
} // namespace hermit
