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

#include "Hermit/Foundation/Notification.h"
#include "DataPath.h"

namespace hermit {
	namespace datastore {
		
		//
		bool DataPath::AppendPathComponent(const HermitPtr& h_, const std::string& name, DataPathPtr& outNewPath) {
			NOTIFY_ERROR(h_, "DataPath::AppendPathComponent not implemented.");
			return false;
		}
		
		//
		void DataPath::GetStringRepresentation(const HermitPtr& h_, std::string& outStringRepresentation) {
			NOTIFY_ERROR(h_, "DataPath::GetStringRepresentation not implemented.");
			outStringRepresentation = "";
		}
		
		//
		void DataPath::GetLastPathComponent(const HermitPtr& h_, std::string& outLastPathComponent) {
			NOTIFY_ERROR(h_, "DataPath::GetLastPathComponent not implemented.");
			outLastPathComponent = "";
		}
		
		//
		void StreamOut(const HermitPtr& h_, std::ostream& strm, DataPathPtr arg) {
			std::string pathString;
			arg->GetStringRepresentation(h_, pathString);
			strm << pathString;
		}
		
	} // namespace datastore
} // namespace hermit

