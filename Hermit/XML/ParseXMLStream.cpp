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
#include "Hermit/String/DecodeXMLEntities.h"
#include "ParseXMLStream.h"

namespace hermit {
	namespace xml {
		
		namespace {

			//
			class StreamInCallback : public StreamInXMLDataCallback {
			public:
				//
				StreamInCallback() : mSuccess(false) {
				}
				
				//
				bool Function(const bool& inSuccess, const DataBuffer& inData) {
					mSuccess = inSuccess;
					if (inSuccess) {
						mData.assign(inData.first, inData.second);
					}
					return true;
				}
				
				//
				bool mSuccess;
				std::string mData;
			};
			
			//
			bool FetchXMLData(const StreamInXMLDataFunctionRef& inStreamInFunction, std::string& outXMLData) {
				StreamInCallback callback;
				if (!inStreamInFunction.Call(1024 * 1024, callback)) {
					return false;
				}
				if (!callback.mSuccess) {
					return false;
				}
				outXMLData = callback.mData;
				return true;
			}
			
			//
			class XMLParser {
			public:
				//
				enum E38XMLParseState {
					kE38XMLParseState_Content = 1,
					kE38XMLParseState_Open,
					kE38XMLParseState_Bang,
					kE38XMLParseState_CDATAStart,
					kE38XMLParseState_CDATAStartComplete,
					kE38XMLParseState_CDATAUnknown,
					kE38XMLParseState_CDATA,
					kE38XMLParseState_CDATAEnd1,
					kE38XMLParseState_CDATAEnd2,
					kE38XMLParseState_CommentStart,
					kE38XMLParseState_CommentUnknown,
					kE38XMLParseState_Comment,
					kE38XMLParseState_CommentEnd1,
					kE38XMLParseState_CommentEnd2,
					kE38XMLParseState_StartTag,
					kE38XMLParseState_Attributes,
					kE38XMLParseState_AttributeValue,
					kE38XMLParseState_EndTag
				};
				
				//
				XMLParser(ParseXMLClient& client) : mClient(client) {
				}
				
				//
				ParseXMLStatus XMLParse_OnStart(const HermitPtr& h_,
												const std::string& inStartTag,
												std::string& ioXMLData,
												size_t& ioXMLDataOffset,
												const StreamInXMLDataFunctionRef& inStreamInFunction,
												size_t& ioTotalBytesProcessed,
												int inDepth,
												const std::string& inAttributes,
												bool inIsEmptyElement,
												bool inIsProcessingInstruction) {
					if (inIsProcessingInstruction) {
						return kParseXMLStatus_OK;
					}
					ParseXMLStatus status = mClient.OnStart(inStartTag, inAttributes, inIsEmptyElement);
					if ((status == kParseXMLStatus_OK) && !inIsEmptyElement) {
						status = ParseXMLInternal(h_,
												  ioXMLData,
												  ioXMLDataOffset,
												  inStreamInFunction,
												  ioTotalBytesProcessed,
												  inStartTag,
												  inDepth + 1);
					}
					return status;
				}
				
				//
				ParseXMLStatus XMLParse_OnContent(const HermitPtr& h_, const std::string& inContent) {
					std::string decodedString;
					string::DecodeXMLEntities(h_, inContent, decodedString);
					
					return mClient.OnContent(decodedString);
				}
				
				//
				ParseXMLStatus XMLParse_OnCDATA(const std::string& inCDATA) {
					return kParseXMLStatus_OK;
				}
				
				//
				ParseXMLStatus XMLParse_OnEnd(const std::string& inEndTag) {
					return mClient.OnEnd(inEndTag);
				}
				
				//
				ParseXMLStatus ParseXMLInternal(const HermitPtr& h_,
												std::string& ioXMLData,
												size_t& ioXMLDataOffset,
												const StreamInXMLDataFunctionRef& inStreamInFunction,
												size_t& ioTotalBytesProcessed,
												const std::string& inStartTag,
												int inDepth) {
					E38XMLParseState parseState = kE38XMLParseState_Content;
					bool isEmptyElement = false;
					bool isProcessingInstruction = false;
					std::string startTag;
					std::string endTag;
					std::string attributes;
					char attributeValueDelimiter = 0;
					std::string content;
					std::string cdata;
					std::string cdataStart("CDATA[");
					int cdataPos = 0;
					
					while (true) {
						while (ioXMLDataOffset < ioXMLData.size()) {
							char ch = ioXMLData[ioXMLDataOffset];
							++ioXMLDataOffset;
							++ioTotalBytesProcessed;
							
							if (parseState == kE38XMLParseState_CDATAStartComplete) {
								cdata.clear();
								parseState = kE38XMLParseState_CDATA;
							}
							if (parseState == kE38XMLParseState_CDATA) {
								if (ch == ']') {
									parseState = kE38XMLParseState_CDATAEnd1;
								}
								else {
									cdata.push_back(ch);
								}
							}
							else if (parseState == kE38XMLParseState_CDATAEnd1) {
								if (ch == ']') {
									parseState = kE38XMLParseState_CDATAEnd2;
								}
								else {
									cdata.push_back(']');
									cdata.push_back(ch);
									parseState = kE38XMLParseState_CDATA;
								}
							}
							else if (parseState == kE38XMLParseState_CDATAEnd2) {
								if (ch == '>') {
									if (cdata.size() > 0) {
										ParseXMLStatus status = XMLParse_OnCDATA(cdata);
										if (status != kParseXMLStatus_OK) {
											return status;
										}
									}
									cdata.clear();
									parseState = kE38XMLParseState_Content;
								}
								else {
									cdata.push_back(']');
									cdata.push_back(']');
									cdata.push_back(ch);
									parseState = kE38XMLParseState_CDATA;
								}
							}
							else if (parseState == kE38XMLParseState_CommentStart) {
								if (ch == '-') {
									parseState = kE38XMLParseState_Comment;
								}
								else {
									parseState = kE38XMLParseState_CommentUnknown;
								}
							}
							else if (parseState == kE38XMLParseState_Comment) {
								if (ch == '-') {
									parseState = kE38XMLParseState_CommentEnd1;
								}
							}
							else if (parseState == kE38XMLParseState_CommentEnd1) {
								if (ch == '-') {
									parseState = kE38XMLParseState_CommentEnd2;
								}
								else {
									parseState = kE38XMLParseState_Comment;
								}
							}
							else if (parseState == kE38XMLParseState_CommentEnd2) {
								if (ch == '>') {
									parseState = kE38XMLParseState_Content;
								}
								else {
									parseState = kE38XMLParseState_CommentUnknown;
								}
							}
							else if (ch == '<') {
								if (parseState == kE38XMLParseState_Open) {
									return kParseXMLStatus_Error;
								}
								else if (parseState == kE38XMLParseState_Content) {
									if (content.size() > 0) {
										ParseXMLStatus status = XMLParse_OnContent(h_, content);
										if (status != kParseXMLStatus_OK) {
											return status;
										}
										content.clear();
									}
								}
								parseState = kE38XMLParseState_Open;
							}
							else if (ch == '/') {
								if (parseState == kE38XMLParseState_Open) {
									parseState = kE38XMLParseState_EndTag;
									endTag.clear();
								}
								else if ((parseState == kE38XMLParseState_StartTag) ||
										 (parseState == kE38XMLParseState_Attributes)) {
									isEmptyElement = true;
								}
								else if (parseState == kE38XMLParseState_Content) {
									content.push_back(ch);
								}
								else if (parseState == kE38XMLParseState_AttributeValue) {
									attributes.push_back(ch);
								}
							}
							else if (ch == '>') {
								if ((parseState == kE38XMLParseState_StartTag) ||
									(parseState == kE38XMLParseState_Attributes)) {
									ParseXMLStatus status = XMLParse_OnStart(h_,
																			 startTag,
																			 ioXMLData,
																			 ioXMLDataOffset,
																			 inStreamInFunction,
																			 ioTotalBytesProcessed,
																			 inDepth,
																			 attributes,
																			 isEmptyElement,
																			 isProcessingInstruction);
									
									if (status != kParseXMLStatus_OK) {
										return status;
									}
									isProcessingInstruction = false;
									parseState = kE38XMLParseState_Content;
								}
								else if (parseState == kE38XMLParseState_EndTag) {
									if (endTag != inStartTag) {
										NOTIFY_ERROR(h_,
													 "ParseXMLInternal(): Mismatched start tag:", inStartTag,
													 "end tag:", endTag,
													 "at character count:", ioTotalBytesProcessed);
										return kParseXMLStatus_Error;
									}
									return XMLParse_OnEnd(endTag);
								}
								else if (parseState == kE38XMLParseState_AttributeValue) {
									attributes.push_back(ch);
								}
							}
							else if ((ch == ' ') && (parseState == kE38XMLParseState_StartTag)) {
								parseState = kE38XMLParseState_Attributes;
							}
							else if ((ch == '?') && (parseState == kE38XMLParseState_Open)) {
								isProcessingInstruction = true;
							}
							else if ((ch == '!') && (parseState == kE38XMLParseState_Open)) {
								parseState = kE38XMLParseState_Bang;
							}
							else if ((ch == '[') && (parseState == kE38XMLParseState_Bang)) {
								parseState = kE38XMLParseState_CDATAStart;
								cdataPos = 0;
							}
							else if ((ch == '-') && (parseState == kE38XMLParseState_Bang)) {
								parseState = kE38XMLParseState_CommentStart;
							}
							else {
								if (parseState == kE38XMLParseState_CDATAStart) {
									if (ch == cdataStart[cdataPos]) {
										if (cdataPos == 5) {
											parseState = kE38XMLParseState_CDATAStartComplete;
										}
										else {
											cdataPos++;
										}
									}
									else {
										parseState = kE38XMLParseState_CDATAUnknown;
									}
								}
								if (parseState == kE38XMLParseState_Open) {
									parseState = kE38XMLParseState_StartTag;
									startTag.clear();
									attributes.clear();
									isEmptyElement = false;
								}
								if (parseState == kE38XMLParseState_StartTag) {
									startTag.push_back(ch);
								}
								else if (parseState == kE38XMLParseState_Attributes) {
									attributes.push_back(ch);
									if ((ch == '"') || (ch == '\'')) {
										attributeValueDelimiter = ch;
										parseState = kE38XMLParseState_AttributeValue;
									}
								}
								else if (parseState == kE38XMLParseState_AttributeValue) {
									attributes.push_back(ch);
									if (ch == attributeValueDelimiter) {
										parseState = kE38XMLParseState_Attributes;
									}
								}
								else if (parseState == kE38XMLParseState_EndTag) {
									endTag.push_back(ch);
								}
								else if (parseState == kE38XMLParseState_Content) {
									if ((ch >= ' ') || (ch < 0)) {
										content.push_back(ch);
									}
								}
							}
						}
						
						if (!FetchXMLData(inStreamInFunction, ioXMLData)) {
							NOTIFY_ERROR(h_, "ParseXMLInternal(): FetchXMLData failed.");
							return kParseXMLStatus_Error;
						}
						ioXMLDataOffset = 0;
						
						if (ioXMLData.empty()) {
							if (inDepth != 0) {
								NOTIFY_ERROR(h_, "ParseXMLInternal(): Unexpected XML end:", ioTotalBytesProcessed);
								return kParseXMLStatus_Error;
							}
							break;
						}
					}
					return kParseXMLStatus_OK;
				}
				
			private:
				//
				ParseXMLClient& mClient;
			};
			
		} // private namespace
		
		//
		ParseXMLStatus ParseXMLStream(const HermitPtr& h_,
									  const StreamInXMLDataFunctionRef& inStreamFunction,
									  ParseXMLClient& client) {
			XMLParser parser(client);
			std::string xmlData;
			size_t offset = 0;
			size_t totalBytesProcessed = 0;
			ParseXMLStatus status = parser.ParseXMLInternal(h_,
															xmlData,
															offset,
															inStreamFunction,
															totalBytesProcessed,
															"",
															0);
			
			return status;
		}
		
	} // namespace xml
} // namespace hermit
