//
//	Hermit
//	Copyright (C) 2018 Paul Young (aka peymojo)
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
#include <thread>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Impl/SQLiteStringMapImpl.h"
#include "Impl/StatementWrapper.h"
#include "SQLiteStringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		namespace SQLiteStringMap_ExportValues_Impl {
			
			//
			typedef std::map<std::string, value::ValuePtr> ValueMap;
			typedef std::vector<value::ValuePtr> ValueVector;

			//
			bool PerformWork(const HermitPtr& h_, const SQLiteStringMapImplPtr& impl, value::ValuePtr& outValues) {
				std::lock_guard<std::mutex> lock(impl->mMutex);
				
				std::string selectStatementText("SELECT key, value FROM map");
				sqlite3_stmt* selectStatement = nullptr;
				int rc = sqlite3_prepare_v2(impl->mDB,
											selectStatementText.c_str(),
											(int)selectStatementText.size(),
											&selectStatement,
											nullptr);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_prepare_v2 failed, error:", sqlite3_errmsg(impl->mDB));
					return false;
				}
				Impl::StatementWrapper selectStatementWrapper(selectStatement);
				
				auto entriesArray = std::make_shared<value::ArrayValueClassT<ValueVector>>();
				while (true) {
					rc = sqlite3_step(selectStatement);
					if (rc == SQLITE_DONE) {
						break;
					}
					if (rc != SQLITE_ROW) {
						NOTIFY_ERROR(h_, "sqlite3_step failed, error:", sqlite3_errmsg(impl->mDB));
						return false;
					}
					
					const unsigned char* keyTextPtr = sqlite3_column_text(selectStatement, 0);
					if (keyTextPtr == nullptr) {
						NOTIFY_ERROR(h_, "keyTextPtr == nullptr");
						return false;
					}
					const unsigned char* valueTextPtr = sqlite3_column_text(selectStatement, 1);
					if (valueTextPtr == nullptr) {
						NOTIFY_ERROR(h_, "valueTextPtr == nullptr");
						return false;
					}

					ValueMap entry;
					entry.insert(ValueMap::value_type("key", value::StringValue::New((const char*)keyTextPtr)));
					entry.insert(ValueMap::value_type("value", value::StringValue::New((const char*)valueTextPtr)));
					auto entryValues = std::make_shared<value::ObjectValueClassT<ValueMap>>(entry);

					entriesArray->AppendItem(entryValues);
				}
				
				outValues = std::static_pointer_cast<value::Value>(entriesArray);
				return true;
			}
			
		} // namespace SQLiteStringMap_ExportValues_Impl
		using namespace SQLiteStringMap_ExportValues_Impl;

		//
		bool SQLiteStringMap::ExportValues(const HermitPtr &h_, value::ValuePtr &outValues) {
			return PerformWork(h_, mImpl, outValues);
		}

	} // namespace sqlitestringmap
} // namespace hermit
