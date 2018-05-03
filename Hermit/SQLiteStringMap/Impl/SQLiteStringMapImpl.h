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

#ifndef SQLiteStringMapImpl_h
#define SQLiteStringMapImpl_h

#include <memory>
#include <sqlite3.h>
#include <thread>

namespace hermit {
	namespace sqlitestringmap {
		
		//
		class SQLiteStringMapImpl {
		public:
			//
			SQLiteStringMapImpl(sqlite3* db);
			
			//
			~SQLiteStringMapImpl();
			
			//
			sqlite3* mDB;
			std::mutex mMutex;
		};
		typedef std::shared_ptr<SQLiteStringMapImpl> SQLiteStringMapImplPtr;
		
	} // namespace sqlitestringmap
} // namespace hermit

#endif /* SQLiteStringMapImpl_h */
