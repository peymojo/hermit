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

#ifndef PageStoreStringMap_h
#define PageStoreStringMap_h

#include <map>
#include <string>
#include "Hermit/Foundation/AsyncFunction.h"
#include "Hermit/Foundation/Hermit.h"
#include "Hermit/Foundation/TaskQueue.h"
#include "Hermit/Foundation/ThreadLock.h"
#include "Hermit/PageStore/PageStore.h"
#include "Hermit/StringMap/StringMap.h"

namespace hermit {
namespace pagestorestringmap {

//
//
class StringMap
{
public:
	//
	//
	typedef std::map<std::string, std::string> Map;
	Map mEntries;
};

//
//
typedef std::shared_ptr<StringMap> StringMapPtr;

//
//
class StringMapPage {
public:
	//
	//
	StringMapPage()
		:
		mDirty(false)
	{
	}
	
	//
	//
	std::string mKey;
	StringMapPtr mMap;
	bool mDirty;
};

//
//
typedef std::shared_ptr<StringMapPage> StringMapPagePtr;

//
//
enum InitPageStoreStringMapStatus
{
	kInitPageStoreStringMapStatus_Unknown,
	kInitPageStoreStringMapStatus_Success,
	kInitPageStoreStringMapStatus_Canceled,
	kInitPageStoreStringMapStatus_Error
};

//
//
DEFINE_ASYNC_FUNCTION_2A(InitPageStoreStringMapCompletionFunction, HermitPtr, InitPageStoreStringMapStatus);

//
//
enum class LoadStringMapPageStatus
{
	kUnknown,
	kSuccess,
	kCanceled,
	kPageNotFound,
	kError
};

//
//
DEFINE_ASYNC_FUNCTION_2A(LoadStringMapPageCompletionFunction,
						 LoadStringMapPageStatus,
						 StringMapPtr);

//
//
class PageStoreStringMap
	:
	public stringmap::StringMap,
	public TaskQueue,
	public std::enable_shared_from_this<PageStoreStringMap>
{
public:
	//
	//
	PageStoreStringMap(const pagestore::PageStorePtr& inPageStore);
	
	//
	//
	void Init(const HermitPtr& h_, const InitPageStoreStringMapCompletionFunctionPtr& inCompletionFunction);
	
	//
	//
	void LoadPage(const HermitPtr& h_, const std::string& inKey, const LoadStringMapPageCompletionFunctionPtr& inCompletionFunction);
	
	//
	virtual void GetValue(const HermitPtr& h_,
						  const std::string& inKey,
						  const stringmap::GetStringMapValueCompletionFunctionPtr& inCompletionFunction) override;
	
	//
	virtual void SetValue(const HermitPtr& h_,
						  const std::string& inKey,
						  const std::string& inValue,
						  const stringmap::SetStringMapValueCompletionFunctionPtr& inCompletionFunction) override;
	
	//
	virtual void Lock(const HermitPtr& h_,
					  const stringmap::LockStringMapCompletionFunctionPtr& inCompletionFunction) override;
	
	//
	virtual void Unlock(const HermitPtr& h_) override;
	
	//
	virtual void CommitChanges(const HermitPtr& h_,
							   const stringmap::CommitStringMapChangesCompletionFunctionPtr& inCompletionFunction) override;
	
	//
	virtual void Validate(const HermitPtr& h_,
						  const stringmap::ValidateStringMapCompletionFunctionPtr& inCompletionFunction) override;

	//
	//
	static const int kMaximumPageSize;

	//
	//
	typedef std::multimap<std::string, StringMapPagePtr> PageMap;

	//
	//
	pagestore::PageStorePtr mPageStore;
	InitPageStoreStringMapStatus mInitStatus;
	PageMap mPages;
	ThreadLock mLock;
};

} // namespace pagestorestringmap
} // namespace hermit

#endif
