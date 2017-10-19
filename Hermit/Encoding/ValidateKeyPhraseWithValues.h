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

#ifndef ValidateKeyPhraseWithValues_h
#define ValidateKeyPhraseWithValues_h

#include <string>
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Value/Value.h"

namespace hermit {
	namespace encoding {
		
		//
		bool ValidateKeyPhraseWithValues(const HermitPtr& h_,
										 const std::string& inKeyPhrase,
										 const value::ObjectValuePtr& inValues,
										 std::string& outAESKey);
		
	} // namespace encoding
} // namespace hermit

#endif 
