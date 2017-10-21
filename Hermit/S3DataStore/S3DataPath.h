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

#ifndef S3DataPath_h
#define S3DataPath_h

#include <string>
#include "Hermit/DataStore/DataPath.h"

namespace hermit {
	namespace s3datastore {
		
		//
		class S3DataPath : public datastore::DataPath {
		public:
			//
			S3DataPath(const std::string& inPath);
			
			//
			virtual bool AppendPathComponent(const HermitPtr& h_,
											 const std::string& name,
											 datastore::DataPathPtr& outDataPath) override;
			
			//
			virtual void GetStringRepresentation(const HermitPtr& h_, std::string& outStringRepresentation) override;
			
			//
			virtual void GetLastPathComponent(const HermitPtr& h_, std::string& outLastPathComponent) override;
			
			//
			std::string mPath;
		};
		
		//
		typedef std::shared_ptr<S3DataPath> S3DataPathPtr;
		
	} // namespace s3datastore
} // namespace hermit

#endif

