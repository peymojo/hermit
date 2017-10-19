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

#ifndef Filehaven_EnumerateRootJSONValuesCallback_h
#define Filehaven_EnumerateRootJSONValuesCallback_h

#include "Hermit/Foundation/Notification.h"
#include "Hermit/Value/EnumerateDataValuesFunction.h"
#include "Hermit/Value/Value.h"

namespace hermit {
namespace json {

//
template <typename StringT, typename ValueMapT, typename ValueVectorT>
class EnumerateRootJSONValuesCallbackT : public value::EnumerateDataValuesCallback {
public:
	//
	EnumerateRootJSONValuesCallbackT()
		:
		mSuccess(false)
	{
	}
	
	//
	//
	virtual bool Function(const HermitPtr& h_,
						  bool inSuccess,
						  const std::string& inName,
						  const value::DataType& inType,
						  const void* inValue) override {
		mSuccess = inSuccess;
		if (!inSuccess) {
			return false;
		}

		if (inType == value::DataType::kArray) {
			mValue = std::make_shared<value::ArrayValueClassT<ValueVectorT>>();
			value::EnumerateDataValuesFunction* f = (value::EnumerateDataValuesFunction*)inValue;
			value::EnumerateValuesCallbackT<StringT, ValueMapT, ValueVectorT> callback(*mValue);
			if (!f->Call(h_, callback)) {
				if (!callback.mSuccess) {
					NOTIFY_ERROR(h_, "EnumerateRootJSONValuesCallbackT: array value enumeration failed.");
					mSuccess = false;
				}
				return false;
			}
		}
		else if (inType == value::DataType::kObject) {
			mValue = std::make_shared<value::ObjectValueClassT<ValueMapT>>();
			value::EnumerateDataValuesFunction* f = (value::EnumerateDataValuesFunction*)inValue;
			value::EnumerateValuesCallbackT<StringT, ValueMapT, ValueVectorT> callback(*mValue);
			if (!f->Call(h_, callback)) {
				if (!callback.mSuccess) {
					NOTIFY_ERROR(h_, "EnumerateRootJSONValuesCallbackT: object value enumeration failed.");
					mSuccess = false;
				}
				return false;
			}
		}
		else {
			NOTIFY_ERROR(h_, "EnumerateRootJSONValuesCallbackT: unsupported root object type.");
			mSuccess = false;
			return false;
		}
		return true;
	}
	
	//
	//
	bool mSuccess;
	value::ValuePtr mValue;
};

} // namespace json
} // namespace hermit

#endif
