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

#include <sstream>
#include "Hermit/Foundation/Notification.h"
#include "DataValueToJSON.h"

namespace hermit {
	namespace json {
		
		namespace {
			
			//
			std::string EscapeString(const char* inString) {
				std::string result;
				uint64_t len = strlen(inString);
				for (uint64_t i = 0; i < len; ++i) {
					unsigned char ch = inString[i];
					if (ch < 32) {
						if (ch == 8) { // backspace
							result += "\b";
						}
						else if (ch == 9) { // horizontal tab
							result += "\t";
						}
						else if (ch == 10) { // newline (line feed)
							result += "\n";
						}
						else if (ch == 12) { // form feed
							result += "\f";
						}
						else if (ch == 13) { // carriage return
							result += "\r";
						}
						else {
							char buf[16];
							sprintf(buf, "\\u%04x", ch);
							result += buf;
						}
					}
					else {
						if ((ch == '\"') || (ch == '\\')) {
							result += "\\";
						}
						result += ch;
					}
				}
				return result;
			}
			
			//
			class EnumerateDataValuesCallback : public value::EnumerateDataValuesCallback {
			public:
				//
				EnumerateDataValuesCallback() :
				mIsFirstItem(true),
				mErrorEncountered(false) {
				}
				
				//
				virtual bool Function(const HermitPtr& h_,
									  bool inSuccess,
									  const std::string& inName,
									  const value::DataType& inType,
									  const void* inValue) override {
					if (!mIsFirstItem) {
						mStream << ", ";
					}
					mIsFirstItem = false;
					
					if (!inName.empty()) {
						mStream << "\"" << inName << "\" : ";
					}
					if (inType == value::DataType::kInt) {
						mStream << *(const int64_t*)inValue;
					}
					else if (inType == value::DataType::kBool) {
						mStream << (*(const bool*)inValue ? "true" : "false");
					}
					else if (inType == value::DataType::kString) {
						mStream << "\"" << EscapeString((const char*)inValue) << "\"";
					}
					else if (inType == value::DataType::kArray) {
						mStream << "[ ";
						value::EnumerateDataValuesFunction* f = (value::EnumerateDataValuesFunction*)inValue;
						EnumerateDataValuesCallback callback;
						if (!f->Call(h_, callback)) {
							return false;
						}
						mStream << callback.mStream.str();
						mStream << " ]";
					}
					else if (inType == value::DataType::kObject) {
						mStream << "{ ";
						value::EnumerateDataValuesFunction* f = (value::EnumerateDataValuesFunction*)inValue;
						EnumerateDataValuesCallback callback;
						if (!f->Call(h_, callback)) {
							return false;
						}
						mStream << callback.mStream.str();
						mStream << " }";
					}
					else {
						NOTIFY_ERROR(h_, "DataValueToJSON: unrecognized object type");
						mErrorEncountered = true;
						return false;
					}
					return true;
				}
				
				//
				//
				std::ostringstream mStream;
				bool mIsFirstItem;
				bool mErrorEncountered;
			};
			
		} // private namespace
		
		//
		void DataValueToJSON(const HermitPtr& h_, const value::ValuePtr& values, std::string& outJSON) {
			value::EnumerateValuesFunction enumerateValuesFunction(*values);
			EnumerateDataValuesCallback enumCallback;
			if (!enumerateValuesFunction.Call(h_, enumCallback)) {
				//	would like better error reporting here?
				outJSON = "";
				return;
			}
			outJSON = enumCallback.mStream.str();
		}
		
	} // namespace json
} // namespace hermit
