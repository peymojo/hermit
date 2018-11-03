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
		enum class HardLinkInfoResult {
			kUnknown,
			kSuccess,
			kCanceled,
			kStorageFull,
			kError
		};
		
		//
		DEFINE_ASYNC_FUNCTION_6A(ProcessHardLinkCompletion,
								 HermitPtr,
								 HardLinkInfoResult,							// result
								 std::string,									// objectDataId
								 uint64_t,										// dataSize
								 std::string,									// dataHash
								 std::string);									// hashAlgorithm
		
		//
		DEFINE_ASYNC_FUNCTION_2A(ProcessHardLinkFunction,
								 HermitPtr,
								 ProcessHardLinkCompletionPtr);					// completion
		
		//
		DEFINE_ASYNC_FUNCTION_7A(GetHardLinkInfoCompletion,
								 HermitPtr,
								 HardLinkInfoResult,							// result
								 std::vector<std::string>,						// paths
								 std::string,									// objectDataId
								 uint64_t,										// dataSize
								 std::string,									// dataHash
								 std::string);									// hashAlgorithm
		
		//
		DEFINE_ASYNC_FUNCTION_4A(GetHardLinkInfoFunction,
								 HermitPtr,
								 FilePathPtr,									// item
								 ProcessHardLinkFunctionPtr,					// processFunction
								 GetHardLinkInfoCompletionPtr);					// completion
		
		//
		class HardLinkMapImpl;
		typedef std::shared_ptr<HardLinkMapImpl> HardLinkMapImplPtr;
		
		//
		class HardLinkMap : public GetHardLinkInfoFunction, public std::enable_shared_from_this<HardLinkMap> {
		public:
			//
			HardLinkMap(const FilePathPtr& rootItem);
			
			//
			~HardLinkMap();
			
			//
			virtual void Call(const hermit::HermitPtr& h_,
							  const FilePathPtr& item,
							  const ProcessHardLinkFunctionPtr& processFunction,
							  const GetHardLinkInfoCompletionPtr& completion) override;
			
			//
			HardLinkMapImplPtr mImpl;
		};
		typedef std::shared_ptr<HardLinkMap> HardLinkMapPtr;

	} // namespace file
} // namespace hermit

#endif /* HardLinkMap_h */
