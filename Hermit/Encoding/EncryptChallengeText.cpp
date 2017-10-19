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
#include <string.h>
#include "Hermit/Encoding/AES256EncryptCBC.h"
#include "EncryptChallengeText.h"

namespace hermit {
	namespace encoding {
		
		namespace {

			//
			const char* kChallengePlaintext = "pNSC,na)*~Bp{d4d5/P\"t}C,%(FQnDk<Q&HIJJFp#e7BAU851<`zg*>7}'7u#Nrs+XjB.h~i8Ugqyu~YXK1nC-g-$2hoxI<toX<okkLt>\\9CI[Pjg9JY&Wyw\\@>NF*O,Yl]z{E<UqlwnXOc#CN$J.(W;Tj3e^N5Y2`<:<^wSt[.M;tn!K9-`cqLSt|rJS(]wai1(XLH^Aa?:+Im$S,\\v\"N,j,e\\Do&ih^SxwtSfw#6b,eh1%MV4ZQg,}H'm70U6#";
			
		} // private namespace
		
		//
		void EncryptChallengeText(const HermitPtr& h_,
								  const std::string& inKey,
								  const std::string& inInputVector,
								  std::string& outEncryptedText) {
			uint64_t dataLen = strlen(kChallengePlaintext);
			AES256EncryptCBCCallbackClass callback;
			AES256EncryptCBC(h_,
							 DataBuffer(kChallengePlaintext, dataLen),
							 inKey,
							 inInputVector,
							 callback);
			
			if (callback.mStatus != kAES256EncryptCBC_Success) {
				outEncryptedText = "";
				return;
			}
			outEncryptedText = callback.mValue;
		}
		
	} // namespace encoding
} // namespace hermit
