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
#include "Impl/SQLiteStringMapImpl.h"
#include "Impl/StatementWrapper.h"
#include "SQLiteStringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		namespace SQLiteStringMap_GetValue_Impl {
			
			//
			stringmap::GetStringMapValueResult PerformWork(const HermitPtr& h_,
														   const SQLiteStringMapImplPtr& impl,
														   const std::string& key,
														   std::string& outValue) {
				std::lock_guard<std::mutex> lock(impl->mMutex);
				
				std::string selectStatementText("SELECT value FROM map where key = ?");
				sqlite3_stmt* selectStatement = nullptr;
				int rc = sqlite3_prepare_v2(impl->mDB,
											selectStatementText.c_str(),
											(int)selectStatementText.size(),
											&selectStatement,
											nullptr);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_prepare_v2 failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::GetStringMapValueResult::kError;
				}
				Impl::StatementWrapper selectStatementWrapper(selectStatement);
				
				rc = sqlite3_bind_text(selectStatement, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::GetStringMapValueResult::kError;
				}
				
				rc = sqlite3_step(selectStatement);
				if (rc == SQLITE_DONE) {
					return stringmap::GetStringMapValueResult::kEntryNotFound;
				}
				if (rc != SQLITE_ROW) {
					NOTIFY_ERROR(h_, "sqlite3_step failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::GetStringMapValueResult::kError;
				}
				
				const unsigned char* valueTextPtr = sqlite3_column_text(selectStatement, 0);
				if (valueTextPtr == nullptr) {
					NOTIFY_ERROR(h_, "valueTextPtr == nullptr");
					return stringmap::GetStringMapValueResult::kError;
				}
				std::string valueText((const char*)valueTextPtr);
				outValue = valueText;
				return stringmap::GetStringMapValueResult::kEntryFound;
			}

		} // namespace SQLiteStringMap_GetValue_Impl
		using namespace SQLiteStringMap_GetValue_Impl;
		
		//
		void SQLiteStringMap::GetValue(const HermitPtr& h_,
									   const std::string& key,
									   const stringmap::GetStringMapValueCompletionPtr& completion) {
			std::string value;
			auto result = PerformWork(h_, mImpl, key, value);
			completion->Call(h_, result, value);
		}
		
	} // namespace sqlitestringmap
} // namespace hermit

