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

#include <map>
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Hermit/String/StringToUInt64.h"
#include "EnumerateRootJSONValuesCallback.h"
#include "JSONToDataValue.h"

namespace hermit {
	namespace json {
		
		namespace {
			
			//
			typedef std::vector<value::ValuePtr> ValueVector;
			typedef std::map<std::string, value::ValuePtr> ValueMap;

			//
			bool IsWhitespace(char inCh) {
				return (inCh == '\r') || (inCh == '\n') || (inCh == '\t') || (inCh == ' ');
			}
			
			//
			bool IsBoolVal(const std::string& inCurrentBoolStr, char inCh) {
				if (inCurrentBoolStr.empty()) {
					return ((inCh == 't') || (inCh == 'f'));
				}
				if (inCurrentBoolStr.size() == 1) {
					if (inCurrentBoolStr[0] == 't') {
						return (inCh == 'r');
					}
					if (inCurrentBoolStr[0] == 'f') {
						return (inCh == 'a');
					}
					return false;
				}
				if (inCurrentBoolStr.size() == 2) {
					if (inCurrentBoolStr[0] == 't') {
						return (inCh == 'u');
					}
					if (inCurrentBoolStr[0] == 'f') {
						return (inCh == 'l');
					}
					return false;
				}
				if (inCurrentBoolStr.size() == 3) {
					if (inCurrentBoolStr[0] == 't') {
						return (inCh == 'e');
					}
					if (inCurrentBoolStr[0] == 'f') {
						return (inCh == 's');
					}
					return false;
				}
				if (inCurrentBoolStr.size() == 4) {
					if (inCurrentBoolStr[0] == 'f') {
						return (inCh == 'e');
					}
					return false;
				}
				return false;
			}
			
			//
			bool IsNullVal(const std::string& inCurrentNullStr, char inCh) {
				if (inCurrentNullStr.empty()) {
					return (inCh == 'n');
				}
				if (inCurrentNullStr.size() == 1) {
					return (inCh == 'u');
				}
				if (inCurrentNullStr.size() == 2) {
					return (inCh == 'l');
				}
				if (inCurrentNullStr.size() == 3) {
					return (inCh == 'l');
				}
				return false;
			}
			
			//
			bool IsNumberVal(char inCh) {
				return (((inCh >= '0') && (inCh <= '9'))
						|| (inCh == '.')
						|| (inCh == '-')
						|| (inCh == '+')
						|| (inCh == 'e')
						|| (inCh == 'E'));
			}
			
			//
			bool IsHex(char inCh) {
				return (((inCh >= '0') && (inCh <= '9'))
						|| ((inCh >= 'a') && (inCh <= 'f'))
						|| ((inCh >= 'A') && (inCh <= 'F')));
			}
			
			//
			int FromHex(char inHex) {
				if ((inHex >= '0') && (inHex <= '9')) {
					return inHex - '0';
				}
				if ((inHex >= 'a') && (inHex <= 'f')) {
					return (inHex - 'a') + 10;
				}
				if ((inHex >= 'A') && (inHex <= 'F')) {
					return (inHex - 'A') + 10;
				}
				return 0;
			}
			
			//
			bool HexSequenceToChar(const HermitPtr& h_, const std::string& inHexSequence, unsigned char& outChar) {
				if (!IsHex(inHexSequence[0]) ||
					!IsHex(inHexSequence[1]) ||
					!IsHex(inHexSequence[2]) ||
					!IsHex(inHexSequence[3])) {
					NOTIFY_ERROR(h_, "HexSequenceToChar: Invalid hex sequence:", inHexSequence);
					return false;
				}
				unsigned char c3 = FromHex(inHexSequence[3]);
				unsigned char c2 = FromHex(inHexSequence[2]);
				unsigned char c1 = FromHex(inHexSequence[1]);
				unsigned char c0 = FromHex(inHexSequence[0]);
				
				if ((c0 != 0) || (c1 != 0)) {
					NOTIFY_ERROR(h_, "HexSequenceToChar: Not sure what to do with hex sequence > 255:", inHexSequence);
					return false;
				}
				unsigned char result = c2 * 16 + c3;
				outChar = result;
				return true;
			}
			
			//
			bool UnescapeString(const HermitPtr& h_, std::string& ioString) {
				std::string result;
				uint64_t len = ioString.size();
				for (uint64_t i = 0; i < len;) {
					unsigned char ch = ioString[i++];
					if (ch == '\\') {
						if (i == len) {
							NOTIFY_ERROR(h_, "UnescapeString: String ends with escape charactger:", ioString);
							return false;
						}
						ch = ioString[i++];
						if (ch == 'u') {
							if ((len - i) < 4) {
								NOTIFY_ERROR(h_, "UnescapeString: \\u sequence clipped at end:", ioString);
								return false;
							}
							std::string hexDigits(ioString.substr(i, 4));
							i += 4;
							
							if (!HexSequenceToChar(h_, hexDigits, ch)) {
								NOTIFY_ERROR(h_, "UnescapeString: Unexpected hex sequence in:", ioString);
								NOTIFY_ERROR(h_, "-- offset:", i - 6);
								return false;
							}
						}
						else if (ch == 'b') {
							ch = 8;
						}
						else if (ch == 'f') {
							ch = 12;
						}
						else if (ch == 'n') {
							ch = 10;
						}
						else if (ch == 'r') {
							ch = 13;
						}
						else if (ch == 't') {
							ch = 9;
						}
					}
					result += ch;
				}
				ioString.swap(result);
				return true;
			}
			
			//
			class JSONParser {
			public:
				//
				JSONParser(const char* input, size_t inputSize) :
				mInput(input),
				mInputSize(inputSize),
				mInString(false),
				mInNumber(false),
				mInBool(false),
				mInNull(false),
				mLastCharWasEscape(false),
				mStringStartPos(0),
				mExpectingKey(false),
				mExpectingValue(false),
				mIndex(0),
				mError(false) {
				}
				
				//
				bool PushValue(const HermitPtr& h_,
							   const value::DataType& inContainerType,
							   value::EnumerateDataValuesCallback& inValuesCallback,
							   const value::DataType& inDataType,
							   const void* inValuePtr) {
					if (inContainerType == value::DataType::kArray) {
						if (!inValuesCallback.Call(h_, true, "", inDataType, inValuePtr)) {
							return false;
						}
					}
					else if (inContainerType == value::DataType::kObject) {
						if (mLastKey.empty()) {
							NOTIFY_ERROR(h_, "PushValue: mLastKey.empty(), mIndex:", mIndex);
							mError = true;
							return false;
						}
						std::string key(mLastKey);
						mLastKey.clear();
						mExpectingKey = true;
						if (!inValuesCallback.Call(h_, true, key, inDataType, inValuePtr)) {
							return false;
						}
						if (!mLastKey.empty()) {
							NOTIFY_ERROR(h_, "PushValue: !mLastKey.empty(), mIndex:", mIndex);
							mError = true;
							return false;
						}
					}
					else {
						NOTIFY_ERROR(h_, "PushValue: unsupported inContainerType, mIndex:", mIndex);
						mError = true;
						return false;
					}
					return true;
				}
				
				//
				class ValuesFunction : public value::EnumerateDataValuesFunction {
				public:
					//
					ValuesFunction(const value::DataType& inContainerType, JSONParser& inParser) :
					mContainerType(inContainerType),
					mParser(inParser) {
					}
					
					//
					bool Function(const HermitPtr& h_, value::EnumerateDataValuesCallback& inCallback) const {
						return mParser.Parse(h_, false, mContainerType, inCallback);
					}
					
					//
					value::DataType mContainerType;
					JSONParser& mParser;
				};
				
				//
				bool Parse(const HermitPtr& h_,
						   const bool& inIsRoot,
						   const value::DataType& inContainerType,
						   value::EnumerateDataValuesCallback& inValuesCallback) {
					while (mIndex < mInputSize) {
						char ch = mInput[mIndex++];
						
						bool thisCharIsEscape = false;
						if (mInString) {
							if (ch == '\"') {
								if (!mLastCharWasEscape) {
									std::string stringValue = std::string(mInput + mStringStartPos,
																		  mIndex - mStringStartPos - 1);
									if (!UnescapeString(h_, stringValue)) {
										NOTIFY_ERROR(h_, "ParseJSON: UnescapeString failed for stringValue:", stringValue);
										mError = true;
										return false;
									}
									if (mExpectingKey) {
										mLastKey = stringValue;
										mExpectingKey = false;
									}
									else {
										if (!PushValue(h_,
													   inContainerType,
													   inValuesCallback,
													   value::DataType::kString,
													   stringValue.c_str())) {
											return false;
										}
									}
									mInString = false;
								}
							}
							else if (ch == '\\') {
								if (!mLastCharWasEscape) {
									thisCharIsEscape = true;
								}
							}
						}
						else if (ch == '\"') {
							if (mInBool) {
								NOTIFY_ERROR(h_, "ParseJSON: mInBool (string begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNumber) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNumber (string begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNull) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNull (string begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							mInString = true;
							mStringStartPos = mIndex;
							if (mExpectingValue) {
								mExpectingValue = false;
							}
						}
						else if (ch == '{') {
							if (mInBool) {
								NOTIFY_ERROR(h_, "ParseJSON: mInBool (object begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNumber) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNumber (object begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNull) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNull (object begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mExpectingValue) {
								mExpectingValue = false;
							}
							mExpectingKey = true;
							
							ValuesFunction objectFunction(value::DataType::kObject, *this);
							if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kObject, &objectFunction)) {
								return false;
							}
							if (inIsRoot) {
								return true;
							}
						}
						else if (ch == '}') {
							if (mInBool) {
								bool boolValue = false;
								if (mCurrentBoolStr == "true") {
									boolValue = true;
								}
								else if (mCurrentBoolStr != "false") {
									NOTIFY_ERROR(h_, "ParseJSON: unexpected bool value, mIndex:", mIndex);
									mError = true;
									return false;
								}
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kBool, &boolValue)) {
									return false;
								}
								mCurrentBoolStr.clear();
								mInBool = false;
							}
							if (mInNumber) {
								std::uint64_t value = string::StringToUInt64(mCurrentNumberStr);
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kInt, &value)) {
									return false;
								}
								mCurrentNumberStr.clear();
								mInNumber = false;
							}
							if (mInNull) {
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kNull, nullptr)) {
									return false;
								}
								mCurrentNullStr.clear();
								mInNull = false;
							}
							return true;
						}
						else if (ch == '[') {
							if (mInBool) {
								NOTIFY_ERROR(h_, "ParseJSON: mInBool (array begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNumber) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNumber (array begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNull) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNull (array begin case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mExpectingValue) {
								mExpectingValue = false;
							}
							
							ValuesFunction arrayFunction(value::DataType::kArray, *this);
							if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kArray, &arrayFunction)) {
								return false;
							}
							if (inIsRoot) {
								return true;
							}
						}
						else if (ch == ']') {
							if (mInBool) {
								bool boolValue = false;
								if (mCurrentBoolStr == "true") {
									boolValue = true;
								}
								else if (mCurrentBoolStr != "false") {
									NOTIFY_ERROR(h_, "ParseJSON: unexpected bool value, mIndex:", mIndex);
									mError = true;
									return false;
								}
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kBool, &boolValue)) {
									return false;
								}
								mCurrentBoolStr.clear();
								mInBool = false;
							}
							if (mInNumber) {
								std::uint64_t value = string::StringToUInt64(mCurrentNumberStr);
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kInt, &value)) {
									return false;
								}
								mCurrentNumberStr.clear();
								mInNumber = false;
							}
							if (mInNull) {
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kNull, nullptr)) {
									return false;
								}
								mCurrentNullStr.clear();
								mInNull = false;
							}
							if ((inContainerType == value::DataType::kArray) && !mLastKey.empty()) {
								//	A string value in the array, backtrack on what we thought was going to be a key.
								std::string value(mLastKey);
								mLastKey.clear();
								if (!PushValue(h_,
											   inContainerType,
											   inValuesCallback,
											   value::DataType::kString,
											   value.c_str())) {
									return false;
								}
							}
							
							return true;
						}
						else if (ch == ':') {
							if (mInBool) {
								NOTIFY_ERROR(h_, "ParseJSON: mInBool (value delimiter case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNumber) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNumber (value delimiter case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInNull) {
								NOTIFY_ERROR(h_, "ParseJSON: mInNull (value delimiter case), mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mExpectingValue) {
								NOTIFY_ERROR(h_, "ParseJSON: double mExpectingValue?, mIndex:", mIndex);
								mError = true;
								return false;
							}
							mExpectingValue = true;
						}
						else if (ch == ',') {
							if (mExpectingValue) {
								NOTIFY_ERROR(h_, "ParseJSON: mExpectingValue, mIndex:", mIndex);
								mError = true;
								return false;
							}
							if (mInBool) {
								bool boolValue = false;
								if (mCurrentBoolStr == "true") {
									boolValue = true;
								}
								else if (mCurrentBoolStr != "false") {
									NOTIFY_ERROR(h_, "ParseJSON: unexpected bool value, mIndex:", mIndex);
									mError = true;
									return false;
								}
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kBool, &boolValue)) {
									return false;
								}
								mCurrentBoolStr.clear();
								mInBool = false;
							}
							if (mInNumber) {
								std::uint64_t value = string::StringToUInt64(mCurrentNumberStr);
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kInt, &value)) {
									return false;
								}
								mCurrentNumberStr.clear();
								mInNumber = false;
							}
							if (mInNull) {
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kNull, nullptr)) {
									return false;
								}
								mCurrentNullStr.clear();
								mInNull = false;
							}
							if ((inContainerType == value::DataType::kArray) && !mLastKey.empty()) {
								//	A string value in the array, backtrack on what we thought was going to be a key.
								std::string value(mLastKey);
								mLastKey.clear();
								if (!PushValue(h_,
											   inContainerType,
											   inValuesCallback,
											   value::DataType::kString,
											   value.c_str())) {
									return false;
								}
							}
							mExpectingKey = true;
						}
						else if (IsBoolVal(mCurrentBoolStr, ch)) {
							if (mExpectingValue) {
								mExpectingValue = false;
								mInBool = true;
							}
							else if (!mInBool) {
								NOTIFY_ERROR(h_, "ParseJSON: unexpected bool value? mIndex:", mIndex);
								mError = true;
								return false;
							}
							mCurrentBoolStr += ch;
						}
						else if (IsNullVal(mCurrentNullStr, ch)) {
							if (mExpectingValue) {
								mExpectingValue = false;
								mInNull = true;
							}
							else if (!mInNull) {
								NOTIFY_ERROR(h_, "ParseJSON: unexpected null value? mIndex:", mIndex);
								mError = true;
								return false;
							}
							mCurrentNullStr += ch;
						}
						else if (IsNumberVal(ch)) {
							if (mExpectingValue) {
								mExpectingValue = false;
								mInNumber = true;
							}
							else if (!mInNumber) {
								NOTIFY_ERROR(h_, "ParseJSON: unexpected number value? mIndex:", mIndex);
								mError = true;
								return false;
							}
							mCurrentNumberStr += ch;
						}
						else if (IsWhitespace(ch)) {
							if (mInBool) {
								bool boolValue = false;
								if (mCurrentBoolStr == "true") {
									boolValue = true;
								}
								else if (mCurrentBoolStr != "false") {
									NOTIFY_ERROR(h_, "ParseJSON: unexpected bool value, mIndex:", mIndex);
									mError = true;
									return false;
								}
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kBool, &boolValue)) {
									return false;
								}
								mCurrentBoolStr.clear();
								mInBool = false;
							}
							if (mInNumber) {
								std::uint64_t value = string::StringToUInt64(mCurrentNumberStr);
								if (!PushValue(h_, inContainerType, inValuesCallback, value::DataType::kInt, &value)) {
									return false;
								}
								mCurrentNumberStr.clear();
								mInNumber = false;
							}
						}
						else {
							NOTIFY_ERROR(h_, "ParseJSON: !IsWhitespace(ch), mIndex:", mIndex);
							mError = true;
							return false;
						}
						
						mLastCharWasEscape = thisCharIsEscape;
					}
					
					NOTIFY_ERROR(h_, "ParseJSON: Unexpected end.");
					return false;
				}
				
				//
				const char* mInput;
				size_t mInputSize;
				bool mInString;
				bool mInBool;
				std::string mCurrentBoolStr;
				bool mInNumber;
				std::string mCurrentNumberStr;
				bool mInNull;
				std::string mCurrentNullStr;
				bool mLastCharWasEscape;
				uint64_t mStringStartPos;
				bool mExpectingKey;
				bool mExpectingValue;
				std::string mLastKey;
				uint64_t mIndex;
				bool mError;
			};
			
		} // private namespace
		
		//
		bool JSONToDataValue(const HermitPtr& h_, const char* input, size_t inputSize, value::ValuePtr& outValue, uint64_t& outBytesConsumed) {
			EnumerateRootJSONValuesCallbackT<std::string, ValueMap, ValueVector> valuesCallback;
			JSONParser parser(input, inputSize);
			if (!parser.Parse(h_, true, value::DataType::kArray, valuesCallback)) {
				if (parser.mError) {
					NOTIFY_ERROR(h_, "JSONToDataValue: JSONParser.Parse failed");
					return false;
				}
				// TODO: In a previous implementation which used client callbacks, the client could abort the json parse once it
				// got the information it needed. This isn't still true, so this combination of Parse returning false but mError not
				// being set seems obsolete. (Also there was a confusing side effect with that model where the outBytesConsumed
				// would be less than the end of the json.)
				NOTIFY_ERROR(h_, "JSONToDataValue: JSONParser.Parse returned false but no error raised?");
			}
			outValue = valuesCallback.mValue;
			outBytesConsumed = parser.mIndex;
			return true;
		}
		
	} // namespace json
} // namespace hermit
