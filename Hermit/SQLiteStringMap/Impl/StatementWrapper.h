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

#ifndef StatementWrapper_h
#define StatementWrapper_h

#include <sqlite3.h>

namespace hermit {
	namespace sqlitestringmap {
		namespace Impl {
			
			//
			class StatementWrapper {
			public:
				//
				StatementWrapper() : mStatement(nullptr) {
				}
				
				//
				StatementWrapper(sqlite3_stmt* statement) : mStatement(statement) {
				}
				
				//
				~StatementWrapper() {
					if (mStatement != nullptr) {
						sqlite3_finalize(mStatement);
					}
				}
				
				//
				sqlite3_stmt* mStatement;
			};
			
		} // namespace Impl
	} // namespace sqlitestringmap
} // namespace hermit

#endif /* StatementWrapper_h */
