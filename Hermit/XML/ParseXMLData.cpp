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
#include "ParseXMLStream.h"
#include "ParseXMLData.h"

namespace hermit {
	namespace xml {
		
		namespace {

			//
			class StreamInDataClass : public StreamInXMLDataFunction {
			public:
				//
				StreamInDataClass(const std::string& inData) : mData(inData), mOffset(0) {
				}
				
				//
				bool Function(const uint64_t& inRequestedBytes, const StreamInXMLDataCallbackRef& inCallback) {
					uint64_t totalBytes = mData.size();
					uint64_t bytesRemaining = totalBytes - mOffset;
					uint64_t bytesToSend = std::min(inRequestedBytes, bytesRemaining);
					bool result = inCallback.Call(true, DataBuffer(mData.data() + mOffset, bytesToSend));
					mOffset += bytesToSend;
					return result;
				}
				
				//
				const std::string& mData;
				uint64_t mOffset;
			};
			
		} // private namespace
		
		//
		ParseXMLStatus ParseXMLData(const HermitPtr& h_, const std::string& xmlData, ParseXMLClient& client) {
			StreamInDataClass streamInData(xmlData);
			return ParseXMLStream(h_, streamInData, client);
		}
		
	} // namespace xml
} // namespace hermit
