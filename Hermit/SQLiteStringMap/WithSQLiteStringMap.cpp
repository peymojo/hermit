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

#include "Hermit/File/AppendToFilePath.h"
#include "Hermit/File/FilePathToCocoaPathString.h"
#include "Hermit/Foundation/Notification.h"
#include "Impl/SQLiteStringMapImpl.h"
#include "Impl/StatementWrapper.h"
#include "WithSQLiteStringMap.h"
#include "SQLiteStringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		
		namespace WithSQLiteStringMap_Impl {
			
			//
			const int kCurrentStringMapDBVersion = 1;
			
			//
			struct HermitWrapper {
				//
				HermitWrapper(const hermit::HermitPtr& h_)
				: mH_(h_) {
				}
				
				//
				hermit::HermitPtr mH_;
			};
			
			//
			static int callback(void* param, int argc, char** argv, char** azColName) {
				HermitWrapper* wrapper = (HermitWrapper*)param;
				if (wrapper != nullptr) {
					NOTIFY_ERROR(wrapper->mH_, "Unexpected callback invocation?");
				}
				return 0;
			}
			
			//
			enum class CreateDatabaseResult {
				kUnknown,
				kSuccess,
				kDatabaseTooOld,
				kError
			};
			
			//
			CreateDatabaseResult CreateDatabase(const hermit::HermitPtr& h_,
												const hermit::file::FilePathPtr& stringMapPath,
												SQLiteStringMapImplPtr& outImpl) {
				
				HermitWrapper hermit(h_);
				
				std::string stringMapDBPath;
				hermit::file::FilePathToCocoaPathString(h_, stringMapPath, stringMapDBPath);
				if (stringMapDBPath.empty()) {
					NOTIFY_ERROR(h_, "FilePathToCocoaPathString failed, path:", stringMapPath);
					return CreateDatabaseResult::kError;
				}
				
				sqlite3* db = nullptr;
				int rc = sqlite3_open(stringMapDBPath.c_str(), &db);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "CreateDatabase: sqlite3_open failed, errmsg:", sqlite3_errmsg(db));
					sqlite3_close(db);
					return CreateDatabaseResult::kError;
				}
				
				auto impl = std::make_shared<SQLiteStringMapImpl>(db);
				
				rc = sqlite3_busy_timeout(db, 3000);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "CreateDatabase: sqlite3_busy_timeout failed.");
					NOTIFY_ERROR(h_, "-- errmsg:", sqlite3_errmsg(db));
					return CreateDatabaseResult::kError;
				}
				
				char* zErrMsg = nullptr;
				rc = sqlite3_exec(db,
								  "CREATE TABLE IF NOT EXISTS config (version INTEGER)",
								  callback,
								  &hermit,
								  &zErrMsg);
				if (rc != SQLITE_OK) {
					NOTIFY_ERROR(h_, "CreateDatabase: create config table failed, error:", zErrMsg);
					sqlite3_free(zErrMsg);
					return CreateDatabaseResult::kError;
				}
				
				int version = 0;
				{
					std::string selectStatementText("SELECT version FROM config");
					sqlite3_stmt* selectStatement = nullptr;
					rc = sqlite3_prepare_v2(db,
											selectStatementText.c_str(),
											(int)selectStatementText.size(),
											&selectStatement,
											nullptr);
					
					if (rc != 0) {
						NOTIFY_ERROR(h_,
									 "CreateDatabase: sqlite3_prepare_v2 failed for select statement, error:",
									 sqlite3_errmsg(db));
						sqlite3_finalize(selectStatement);
						return CreateDatabaseResult::kError;
					}
					Impl::StatementWrapper selectStatementWrapper(selectStatement);
					
					rc = sqlite3_step(selectStatement);
					if (rc == SQLITE_ROW) {
						version = sqlite3_column_int(selectStatement, 0);
					}
					else if (rc != SQLITE_DONE) {
						NOTIFY_ERROR(h_, "CreateDatabase: select failed, error:", sqlite3_errmsg(db));
						return CreateDatabaseResult::kError;
					}
				}
				if ((version > 0) && (version < kCurrentStringMapDBVersion)) {
					NOTIFY_ERROR(h_, "CreateDatabase: unsupported stringmap version:", version);
					return CreateDatabaseResult::kDatabaseTooOld;
				}
				
				rc = sqlite3_exec(db,
								  "CREATE TABLE IF NOT EXISTS map (key TEXT PRIMARY KEY, value TEXT)",
								  callback,
								  &hermit,
								  &zErrMsg);
				if (rc != SQLITE_OK) {
					NOTIFY_ERROR(h_, "CreateDatabase: create map table failed, error:", zErrMsg);
					sqlite3_free(zErrMsg);
					return CreateDatabaseResult::kError;
				}
				
				if (version == 0) {
					std::string insertStatementText("INSERT INTO config (version) VALUES (?)");
					sqlite3_stmt* insertStatement = nullptr;
					rc = sqlite3_prepare_v2(db,
											insertStatementText.c_str(),
											(int)insertStatementText.size(),
											&insertStatement,
											nullptr);
					
					if (rc != 0) {
						NOTIFY_ERROR(h_, "CreateDatabase: sqlite3_prepare_v2 failed for insertStatement, error:", sqlite3_errmsg(db));
						sqlite3_finalize(insertStatement);
						return CreateDatabaseResult::kError;
					}
					Impl::StatementWrapper insertStatementWrapper(insertStatement);
					
					rc = sqlite3_bind_int(insertStatement, 1, kCurrentStringMapDBVersion);
					if (rc != 0) {
						NOTIFY_ERROR(h_,
									 "CreateDatabase: sqlite3_bind_int(1) failed for insertStatement, error:",
									 sqlite3_errmsg(db));
						return CreateDatabaseResult::kError;
					}
					
					rc = sqlite3_step(insertStatement);
					if (rc != SQLITE_DONE) {
						NOTIFY_ERROR(h_,
									 "CreateDatabase: sqlite3_step failed for insertStatement, error:",
									 sqlite3_errmsg(db));
						return CreateDatabaseResult::kError;
					}
					
					int rows = sqlite3_changes(db);
					if (rows != 1) {
						NOTIFY_ERROR(h_, "CreateDatabase: rows != 1? for config insert");
						return CreateDatabaseResult::kError;
					}
				}
				
				outImpl = impl;
				return CreateDatabaseResult::kSuccess;
			}
			
		} // namespace WithSQLiteStringMap_Impl
		using namespace WithSQLiteStringMap_Impl;

		//
		stringmap::WithStringMapResult WithSQLiteStringMap(const HermitPtr& h_,
														   const file::FilePathPtr& filePath,
														   stringmap::StringMapPtr& outStringMap) {
			hermit::file::FilePathPtr dbPath;
			hermit::file::AppendToFilePath(h_, filePath, "stringmap.db", dbPath);
			if (dbPath == nullptr) {
				NOTIFY_ERROR(h_, "AppendToFilePath failed, path:", filePath);
				return stringmap::WithStringMapResult::kError;
			}
			
			SQLiteStringMapImplPtr impl;
			auto result = CreateDatabase(h_, dbPath, impl);
			if (result == CreateDatabaseResult::kDatabaseTooOld) {
				return stringmap::WithStringMapResult::kStringMapTooOld;
			}
			if (result != CreateDatabaseResult::kSuccess) {
				NOTIFY_ERROR(h_, "CreateDatabase failed, path:", filePath);
				return stringmap::WithStringMapResult::kError;
			}
			
			outStringMap = std::make_shared<SQLiteStringMap>(impl);
			return stringmap::WithStringMapResult::kSuccess;
		}
		
	} // namespace pagestorestringmap
} // namespace hermit
