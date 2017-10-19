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

#include "SanitizeXML.h"

namespace e38
{

namespace
{
	//
	//
	bool IsAlpha(
		char inC)
	{
		return ((inC >= 'A') && (inC <= 'Z')) || ((inC >= 'a') && (inC <= 'z'));
	}
	
	//
	//
	bool IsNumber(
		char inC)
	{
		return ((inC >= '0') && (inC <= '9'));
	}
	
} // private namespace

//
//	[4]   	NameStartChar  ::=   	":" | [A-Z] | "_" | [a-z] | [#xC0-#xD6] | [#xD8-#xF6] | [#xF8-#x2FF] 
//									| [#x370-#x37D] | [#x37F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] 
//									| [#x2C00-#x2FEF] | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] 
//									| [#x10000-#xEFFFF]
//	[4a]   	NameChar	   ::=   	NameStartChar | "-" | "." | [0-9] | #xB7 | [#x0300-#x036F] | [#x203F-#x2040]
//
void SanitizeXMLElementName(
	Hermit& inHermit,
	const std::string& inString,
	std::string& outResult)
{
	std::string result;
	if (inString.empty())
	{
		result = "___";
	}
	else
	{
		if (!IsAlpha(inString[0]))
		{
			result = '_';
		}
		result += inString[0];

		for (std::string::size_type i = 1; i < inString.size(); ++i)
		{
			std::string::value_type ch = inString[i];
			if (IsAlpha(ch) || IsNumber(ch) || (ch == '_') || (ch == '-') || (ch == '.'))
			{
				result += ch;
			}
			else
			{
				result += '_';
			}
		}
	}
	outResult = result;
}
	
//
//	[14]   	CharData	   ::=   	[^<&]* - ([^<&]* ']]>' [^<&]*)
//
void SanitizeXMLValue(
	Hermit& inHermit,
	const std::string& inString,
	std::string& outResult)
{
	std::string result;
	for (std::string::size_type i = 0; i < inString.size(); ++i)
	{
		std::string::value_type ch = inString[i];
		if (ch == '&')
		{
			result += "&amp;";
		}
		else if (ch == '<')
		{
			result += "&lt;";
		}
		else if (ch == '>')
		{
			result += "&gt;";
		}
		else
		{
			result += ch;
		}
	}
	outResult = result;
}

} // namespace e38

