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

#ifndef CommandLineTool_h
#define CommandLineTool_h

#include <list>
#include <string>
#include <memory>
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	
	//
	class CommandLineTool {
	public:
		//
		virtual ~CommandLineTool();
		
		//
		virtual void Usage() const = 0;
		
		//
		virtual int Run(const HermitPtr& h_, const std::list<std::string>& args) = 0;
	};
	
	//
	typedef std::shared_ptr<CommandLineTool> CommandLineToolPtr;
	
} // namespace hermit

#endif
