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
			void PerformWork(const HermitPtr& h_,
							 const SQLiteStringMapImplPtr& impl,
							 const std::string& key,
							 const stringmap::GetStringMapValueCompletionFunctionPtr& completion) {
				std::string selectStatementText("SELECT value FROM map where key = ?");
				sqlite3_stmt* selectStatement = nullptr;
				int rc = sqlite3_prepare_v2(impl->mDB,
											selectStatementText.c_str(),
											(int)selectStatementText.size(),
											&selectStatement,
											nullptr);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_prepare_v2 failed, error:", sqlite3_errmsg(impl->mDB));
					completion->Call(h_, stringmap::GetStringMapValueResult::kError, "");
					return;
				}
				Impl::StatementWrapper selectStatementWrapper(selectStatement);
				
				rc = sqlite3_bind_text(selectStatement, 1, key.c_str(), (int)key.size(), SQLITE_TRANSIENT);
				if (rc != 0) {
					NOTIFY_ERROR(h_, "sqlite3_bind_text(1) failed, error:", sqlite3_errmsg(impl->mDB));
					completion->Call(h_, stringmap::GetStringMapValueResult::kError, "");
					return;
				}
				
				rc = sqlite3_step(selectStatement);
				if (rc == SQLITE_DONE) {
					completion->Call(h_, stringmap::GetStringMapValueResult::kEntryNotFound, "");
					return;
				}
				if (rc != SQLITE_ROW) {
					NOTIFY_ERROR(h_, "sqlite3_step failed, error:", sqlite3_errmsg(impl->mDB));
					completion->Call(h_, stringmap::GetStringMapValueResult::kError, "");
					return;
				}
				
				const unsigned char* valueTextPtr = sqlite3_column_text(selectStatement, 0);
				if (valueTextPtr == nullptr) {
					NOTIFY_ERROR(h_, "valueTextPtr == nullptr");
					completion->Call(h_, stringmap::GetStringMapValueResult::kError, "");
					return;
				}
				std::string valueText((const char*)valueTextPtr);
				completion->Call(h_, stringmap::GetStringMapValueResult::kEntryFound, valueText);
			}

			//
			class Task : public AsyncTask {
			public:
				//
				Task(const SQLiteStringMapImplPtr& impl,
					 const std::string& key,
					 const stringmap::GetStringMapValueCompletionFunctionPtr& completion) :
				mImpl(impl),
				mKey(key),
				mCompletion(completion) {
				}
				
				//
				virtual void PerformTask(const HermitPtr& h_) override {
					PerformWork(h_, mImpl, mKey, mCompletion);
				}
				
				//
				SQLiteStringMapImplPtr mImpl;
				std::string mKey;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletion;
			};

			//
			class CompletionProxy : public stringmap::GetStringMapValueCompletionFunction {
			public:
				//
				CompletionProxy(const SQLiteStringMapImplPtr& impl,
								const stringmap::GetStringMapValueCompletionFunctionPtr& completion) :
				mImpl(impl),
				mCompletion(completion) {
				}
				
				//
				virtual void Call(const HermitPtr& h_,
								  const stringmap::GetStringMapValueResult& result,
								  const std::string& value) override {
					mCompletion->Call(h_, result, value);
					mImpl->TaskComplete();
				}
				
				//
				SQLiteStringMapImplPtr mImpl;
				stringmap::GetStringMapValueCompletionFunctionPtr mCompletion;
			};

		} // namespace SQLiteStringMap_GetValue_Impl

		using namespace SQLiteStringMap_GetValue_Impl;
		
		//
		void SQLiteStringMap::GetValue(const HermitPtr& h_,
									   const std::string& key,
									   const stringmap::GetStringMapValueCompletionFunctionPtr& completion) {
			auto completionProxy = std::make_shared<CompletionProxy>(mImpl, completion);
			auto task = std::make_shared<Task>(mImpl, key, completionProxy);
			if (!mImpl->QueueTask(h_, task)) {
				NOTIFY_ERROR(h_, "mImpl->QueueTask failed");
				completion->Call(h_, stringmap::GetStringMapValueResult::kError, "");
			}
		}
		
	} // namespace sqlitestringmap
} // namespace hermit

