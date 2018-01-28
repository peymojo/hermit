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
			void PerformWork(const HermitPtr& h_,
							 const SQLiteStringMapImplPtr& impl,
							 const std::string& inKey,
							 const std::string& inValue,
							 const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletion) {
				std::string updateStmtText("UPDATE map set value = ? where key = ?");
				sqlite3_stmt* updateStmt = nullptr;
				int rc = sqlite3_prepare_v2(impl->mDB,
											updateStmtText.c_str(),
											(int)updateStmtText.size(),
											&updateStmt,
											nullptr);
				
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_prepare_v2(update) failed, error:", sqlite3_errmsg(impl->mDB));
					inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
					return;
				}
				Impl::StatementWrapper updateStmtWrapper(updateStmt);
				
				rc = sqlite3_bind_text(updateStmt, 1, inValue.c_str(), (int)inValue.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
					inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
					return;
				}
				rc = sqlite3_bind_text(updateStmt, 2, inKey.c_str(), (int)inKey.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(2) failed, error:", sqlite3_errmsg(impl->mDB));
					inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
					return;
				}
				
				rc = sqlite3_step(updateStmt);
				if (rc != SQLITE_DONE) {
					NOTIFY_ERROR(h_, "sqlite3_step(update) failed, error:", sqlite3_errmsg(impl->mDB));
					inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
					return;
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
						inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
						return;
					}
					Impl::StatementWrapper insertStmtWrapper(insertStmt);
					
					rc = sqlite3_bind_text(insertStmt, 1, inKey.c_str(), (int)inKey.size(), SQLITE_TRANSIENT);
					if (rc != 0) {
						NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
						inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
						return;
					}
					rc = sqlite3_bind_text(insertStmt, 2, inValue.c_str(), (int)inValue.size(), SQLITE_TRANSIENT);
					if (rc != 0) {
						NOTIFY_ERROR(h_, "sqlite3_bind_text(2) failed, error:", sqlite3_errmsg(impl->mDB));
						inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
						return;
					}
					
					rc = sqlite3_step(insertStmt);
					if (rc != SQLITE_DONE) {
						NOTIFY_ERROR(h_, "sqlite3_step(insert) failed, error:", sqlite3_errmsg(impl->mDB));
						inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
						return;
					}
					
					int rows = sqlite3_changes(impl->mDB);
					if (rows != 1) {
						NOTIFY_ERROR(h_, "Insert failed to add a row");
						inCompletion->Call(h_, stringmap::SetStringMapValueResult::kError);
						return;
					}
				}
				inCompletion->Call(h_, stringmap::SetStringMapValueResult::kSuccess);
			}

			//
			class Task : public AsyncTask {
			public:
				//
				Task(const SQLiteStringMapImplPtr& impl,
					 const std::string& key,
					 const std::string& value,
					 const stringmap::SetStringMapValueCompletionFunctionPtr& completion) :
				mImpl(impl),
				mKey(key),
				mValue(value),
				mCompletion(completion) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_, mImpl, mKey, mValue, mCompletion);
				}
				
				//
				SQLiteStringMapImplPtr mImpl;
				std::string mKey;
				std::string mValue;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletion;
			};

			//
			class CompletionProxy : public stringmap::SetStringMapValueCompletionFunction {
			public:
				//
				CompletionProxy(const SQLiteStringMapImplPtr& impl,
								const stringmap::SetStringMapValueCompletionFunctionPtr& completion) :
				mImpl(impl),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_, const stringmap::SetStringMapValueResult& inResult) override {
					mCompletion->Call(h_, inResult);
					mImpl->TaskComplete();
				}
				
				//
				SQLiteStringMapImplPtr mImpl;
				stringmap::SetStringMapValueCompletionFunctionPtr mCompletion;
			};
			
		} // namespace SQLiteStringMap_SetValue_Impl
		using namespace SQLiteStringMap_SetValue_Impl;
		
		//
		void SQLiteStringMap::SetValue(const HermitPtr& h_,
									   const std::string& key,
									   const std::string& value,
									   const stringmap::SetStringMapValueCompletionFunctionPtr& completion) {
			if (key.empty()) {
				NOTIFY_ERROR(h_, "SQLiteStringMap::SetValue: empty key.");
				completion->Call(h_, stringmap::SetStringMapValueResult::kError);
				return;
			}
			if (key[0] < ' ') {
				NOTIFY_ERROR(h_, "SQLiteStringMap::SetValue: invalid key.");
				completion->Call(h_, stringmap::SetStringMapValueResult::kError);
				return;
			}
			
			auto completionProxy = std::make_shared<CompletionProxy>(mImpl, completion);
			auto task = std::make_shared<Task>(mImpl, key, value, completionProxy);
			if (!mImpl->QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "QueueTask failed.");
				completion->Call(h_, stringmap::SetStringMapValueResult::kError);
			}
		}
		
	} // namespace sqlitestringmap
} // namespace hermit

