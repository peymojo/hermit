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

#ifndef ParseXMLFunctions_h
#define ParseXMLFunctions_h

#include <string>

namespace hermit {
	namespace xml {
		
		//
		enum ParseXMLStatus {
			kParseXMLStatus_Unknown = 0,
			kParseXMLStatus_OK = 1,
			kParseXMLStatus_SkipObject = 2,
			kParseXMLStatus_Cancel = 3,
			kParseXMLStatus_Error = -1
		};
		
		//
		class ParseXMLClient {
		public:
			//
			virtual ParseXMLStatus OnStart(const std::string& tag, const std::string& attributes, bool emptyElement) = 0;
			//
			virtual ParseXMLStatus OnContent(const std::string& content) = 0;
			//
			virtual ParseXMLStatus OnEnd(const std::string& tag) = 0;
		};
		
	} // namespace xml
} // namespace hermit

#endif
