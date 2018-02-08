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

#ifndef HardLinkMap_h
#define HardLinkMap_h

#include "Hermit/File/FilePath.h"
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"

namespace hermit {
	namespace file {
		
		//
		enum class HardLinkInfoStatus {
			kUnknown,
			kSuccess,
			kCanceled,
			kStorageFull,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_5A(ProcessHardLinkCompletionFunction,
								 HermitPtr,
								 HardLinkInfoStatus,							// status
								 std::string,									// objectDataId
								 uint64_t,										// dataSize
								 std::string);									// dataHash
		
		//
		DEFINE_ASYNC_FUNCTION_2A(ProcessHardLinkFunction,
								 HermitPtr,
								 ProcessHardLinkCompletionFunctionPtr);			// completion
		
		//
		DEFINE_ASYNC_FUNCTION_6A(GetHardLinkInfoCompletionFunction,
								 HermitPtr,
								 HardLinkInfoStatus,							// status
								 std::vector<std::string>,						// paths
								 std::string,									// objectDataId
								 uint64_t,										// dataSize
								 std::string);									// dataHash
		
		//
		DEFINE_ASYNC_FUNCTION_4A(GetHardLinkInfoFunction,
								 HermitPtr,
								 FilePathPtr,									// item
								 ProcessHardLinkFunctionPtr,					// processFunction
								 GetHardLinkInfoCompletionFunctionPtr);			// completion
		
		//
		class HardLinkMapImpl;
		
		//
		class HardLinkMap : public GetHardLinkInfoFunction {
		public:
			//
			HardLinkMap(const FilePathPtr& rootItem);
			
			//
			~HardLinkMap();
			
			//
			virtual void Call(const hermit::HermitPtr& h_,
							  const FilePathPtr& item,
							  const ProcessHardLinkFunctionPtr& processFunction,
							  const GetHardLinkInfoCompletionFunctionPtr& completion) override;
			
			//
			HardLinkMapImpl* mImpl;
		};

	} // namespace file
} // namespace hermit

#endif /* HardLinkMap_h */
