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

#ifndef SQLiteStringMap_h
#define SQLiteStringMap_h

#include <memory>
#include "Hermit/StringMap/StringMap.h"

namespace hermit {
	namespace sqlitestringmap {
		
		//
		class SQLiteStringMapImpl;
		typedef std::shared_ptr<SQLiteStringMapImpl> SQLiteStringMapImplPtr;

		//
		class SQLiteStringMap : public stringmap::StringMap {
		public:
			//
			SQLiteStringMap(const SQLiteStringMapImplPtr& impl);
			
			//
			~SQLiteStringMap();
			
			//
			virtual void GetValue(const HermitPtr& h_,
								  const std::string& key,
								  const stringmap::GetStringMapValueCompletionPtr& completion) override;
			
			//
			virtual void SetValue(const HermitPtr& h_,
								  const std::string& key,
								  const std::string& value,
								  const stringmap::SetStringMapValueCompletionPtr& completion) override;
			
			//
			virtual bool ExportValues(const HermitPtr& h_, hermit::value::ValuePtr& outValues) override;

			//
			SQLiteStringMapImplPtr mImpl;
		};
		
	} // namespace sqlitestringmap
} // namespace hermit

#endif /* SQLiteStringMap_h */
