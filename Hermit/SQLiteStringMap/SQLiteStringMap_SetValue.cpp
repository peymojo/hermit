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

#include <sqlite3.h>
#include <string>
#include "Hermit/Foundation/Notification.h"
#include "Impl/SQLiteStringMapImpl.h"
#include "Impl/StatementWrapper.h"
#include "SQLiteStringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		
		namespace SQLiteStringMap_SetValue_Impl {

			//
			stringmap::SetStringMapValueResult PerformWork(const HermitPtr& h_,
														   const SQLiteStringMapImplPtr& impl,
														   const std::string& key,
														   const std::string& value) {
				std::lock_guard<std::mutex> lock(impl->mMutex);

				std::string updateStmtText("UPDATE map set value = ? where key = ?");
				sqlite3_stmt* updateStmt = nullptr;
				int rc = sqlite3_prepare_v2(impl->mDB,
											updateStmtText.c_str(),
											(int)updateStmtText.size(),
											&updateStmt,
											nullptr);
				
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_prepare_v2(update) failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::SetStringMapValueResult::kError;
				}
				Impl::StatementWrapper updateStmtWrapper(updateStmt);
				
				rc = sqlite3_bind_text(updateStmt, 1, value.c_str(), (int)value.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::SetStringMapValueResult::kError;
				}
				rc = sqlite3_bind_text(updateStmt, 2, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(2) failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::SetStringMapValueResult::kError;
				}
				
				rc = sqlite3_step(updateStmt);
				if (rc != SQLITE_DONE) {
					NOTIFY_ERROR(h_, "sqlite3_step(update) failed, error:", sqlite3_errmsg(impl->mDB));
					return stringmap::SetStringMapValueResult::kError;
				}
				
				int rows = sqlite3_changes(impl->mDB);
				if (rows == 0) {
					std::string insertStmtText("INSERT into map (key, value) VALUES (?, ?)");
					sqlite3_stmt* insertStmt = nullptr;
					int rc = sqlite3_prepare_v2(impl->mDB,
												insertStmtText.c_str(),
												(int)insertStmtText.size(),
												&insertStmt,
												nullptr);
					
					if (rc != 0) {
						NOTIFY_ERROR(h_, "sqlite3_prepare_v2(insert) failed, error:", sqlite3_errmsg(impl->mDB));
						return stringmap::SetStringMapValueResult::kError;
					}
					Impl::StatementWrapper insertStmtWrapper(insertStmt);
					
					rc = sqlite3_bind_text(insertStmt, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
					if (rc != 0) {
						NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
						return stringmap::SetStringMapValueResult::kError;
					}
					rc = sqlite3_bind_text(insertStmt, 2, value.c_str(), (int)value.size(), SQLITE_TRANSIENT);
					if (rc != 0) {
						NOTIFY_ERROR(h_, "sqlite3_bind_text(2) failed, error:", sqlite3_errmsg(impl->mDB));
						return stringmap::SetStringMapValueResult::kError;
					}
					
					rc = sqlite3_step(insertStmt);
					if (rc != SQLITE_DONE) {
						NOTIFY_ERROR(h_, "sqlite3_step(insert) failed, error:", sqlite3_errmsg(impl->mDB));
						return stringmap::SetStringMapValueResult::kError;
					}
					
					int rows = sqlite3_changes(impl->mDB);
					if (rows != 1) {
						NOTIFY_ERROR(h_, "Insert failed to add a row");
						return stringmap::SetStringMapValueResult::kError;
					}
				}
				return stringmap::SetStringMapValueResult::kSuccess;
			}
			
		} // namespace SQLiteStringMap_SetValue_Impl
		using namespace SQLiteStringMap_SetValue_Impl;
		
		//
		void SQLiteStringMap::SetValue(const HermitPtr& h_,
									   const std::string& key,
									   const std::string& value,
									   const stringmap::SetStringMapValueCompletionPtr& completion) {
			if (key.empty()) {
				NOTIFY_ERROR(h_, "SQLiteStringMap::SetValue: empty key.");
				completion->Call(h_, stringmap::SetStringMapValueResult::kError);
				return;
			}
			
			auto result = PerformWork(h_, mImpl, key, value);
			completion->Call(h_, result);
		}
		
	} // namespace sqlitestringmap
} // namespace hermit

