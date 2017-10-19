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

#include <map>
#include <string>
#include <vector>
#include "Hermit/Foundation/Notification.h"
#include "Value.h"

namespace hermit {
	namespace value {
		
		//
		typedef std::map<std::string, ValuePtr> ValueMap;
		typedef std::vector<ValuePtr> ValueVector;
		
		//
		bool EnumerateOneValue(const HermitPtr& h_,
							   const std::string& inName,
							   const Value& inValue,
							   EnumerateDataValuesCallback& inCallback) {
			DataType dataType = inValue.GetDataType();
			if (dataType == DataType::kString) {
				const StringValue& stringValue = static_cast<const StringValue&>(inValue);
				return inCallback.Call(h_, true, inName, DataType::kString, stringValue.GetValue().c_str());
			}
			if (dataType == DataType::kObject) {
				const ObjectValue& objectValue = static_cast<const ObjectValue&>(inValue);
				EnumerateObjectValuesFunction objectFunction(objectValue);
				return inCallback.Call(h_, true, inName, DataType::kObject, &objectFunction);
			}
			if (dataType == DataType::kArray) {
				const ArrayValue& arrayValue = static_cast<const ArrayValue&>(inValue);
				EnumerateArrayValuesFunction arrayFunction(arrayValue);
				return inCallback.Call(h_, true, inName, DataType::kArray, &arrayFunction);
			}
			if (dataType == DataType::kInt) {
				const IntValue& intValue = static_cast<const IntValue&>(inValue);
				//	can't let client manipulate value, pass a copy
				int64_t value = intValue.mValue;
				return inCallback.Call(h_, true, inName, DataType::kInt, &value);
			}
			if (dataType == DataType::kBool) {
				const BoolValue& boolValue = static_cast<const BoolValue&>(inValue);
				//	can't let client manipulate value, pass a copy
				bool value = boolValue.mValue;
				return inCallback.Call(h_, true, inName, DataType::kBool, &value);
			}
			NOTIFY_ERROR(h_, "EnumerateOneValue: unexpected data type");
			return inCallback.Call(h_, false, "", DataType::kUnknown, nullptr);
		}
		
		//
		bool EnumerateValuesCallback::AddValue(const HermitPtr& h_, const std::string& inName, const ValuePtr& inValue) {
			if (inValue == nullptr) {
				NOTIFY_ERROR(h_, "EnumerateValuesCallback::AddValue: inValue == null");
				return false;
			}
			
			if (mValue.GetDataType() == DataType::kObject) {
				ObjectValue& objectValue = static_cast<ObjectValue&>(mValue);
				objectValue.SetItem(h_, inName, inValue);
			}
			else if (mValue.GetDataType() == DataType::kArray) {
				ArrayValue& arrayValue = static_cast<ArrayValue&>(mValue);
				arrayValue.AppendItem(inValue);
			}
			else {
				NOTIFY_ERROR(h_, "EnumerateValuesCallback::AddValue: I don't know what to do with this value.");
				return false;
			}
			return true;
		}
		
		//
		EnumerateValuesFunction::EnumerateValuesFunction(const Value& inValue) : mValue(inValue) {
		}
		
		//
		bool EnumerateValuesFunction::Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const {
			return EnumerateOneValue(h_, "", mValue, inCallback);
		}
		
		//
		EnumerateValuesFunctionClass::EnumerateValuesFunctionClass(const ValuePtr& inValue) : mValue(inValue) {
		}
		
		//
		bool EnumerateValuesFunctionClass::Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const {
			return EnumerateOneValue(h_, "", *mValue, inCallback);
		}
		
		//
		class DuplicateArrayItems : public ArrayValueVisitor {
		public:
			//
			DuplicateArrayItems() : mErrorOccurred(false) {
			}
			
			//
			virtual bool VisitValue(const HermitPtr& h_, const Value& inValue) override {
				ValuePtr copy(DuplicateValue(h_, inValue));
				if (copy == nullptr) {
					mErrorOccurred = true;
					return false;
				}
				mItems.push_back(copy);
				return true;
			}
			
			//
			ValueVector mItems;
			bool mErrorOccurred;
		};
		
		//
		class DuplicateObjectItems : public ObjectValueVisitor {
		public:
			//
			DuplicateObjectItems() : mErrorOccurred(false) {
			}
			
			//
			virtual bool VisitValue(const HermitPtr& h_, const std::string& inKey, const Value& inValue) override {
				ValuePtr copy(DuplicateValue(h_, inValue));
				if (copy == nullptr) {
					mErrorOccurred = true;
					return false;
				}
				mItems.insert(ValueMap::value_type(inKey, copy));
				return true;
			}
			
			//
			ValueMap mItems;
			bool mErrorOccurred;
		};
		
		//
		ValuePtr DuplicateValue(const HermitPtr& h_, const Value& inValue) {
			DataType dataType = inValue.GetDataType();
			if (dataType == DataType::kBool) {
				const BoolValue& boolValue = static_cast<const BoolValue&>(inValue);
				return std::make_shared<BoolValue>(boolValue.mValue);
			}
			if (dataType == DataType::kInt) {
				const IntValue& intValue = static_cast<const IntValue&>(inValue);
				return std::make_shared<IntValue>(intValue.mValue);
			}
			if (dataType == DataType::kString) {
				const StringValue& stringValue = static_cast<const StringValue&>(inValue);
				return std::make_shared<StringValue>(stringValue.mValue);
			}
			if (dataType == DataType::kArray) {
				const ArrayValue& arrayValue = static_cast<const ArrayValue&>(inValue);
				DuplicateArrayItems duplicateItems;
				arrayValue.VisitItems(h_, duplicateItems);
				if (duplicateItems.mErrorOccurred) {
					NOTIFY_ERROR(h_, "DuplicateValue: arrayValue.VisitItems failed.");
					return nullptr;
				}
				return std::make_shared<ArrayValueClassT<ValueVector>>(duplicateItems.mItems);
			}
			if (dataType == DataType::kObject) {
				const ObjectValue& objectValue = static_cast<const ObjectValue&>(inValue);
				DuplicateObjectItems duplicateItems;
				objectValue.VisitItems(h_, duplicateItems);
				if (duplicateItems.mErrorOccurred) {
					NOTIFY_ERROR(h_, "DuplicateValue: objectValue.VisitItems failed.");
					return nullptr;
				}
				return std::make_shared<ObjectValueClassT<ValueMap>>(duplicateItems.mItems);
			}
			NOTIFY_ERROR(h_, "DuplicateValue: unexpected data type");
			return nullptr;
		}
		
		//
		bool GetStringValue(const HermitPtr& h_,
							const ObjectValuePtr& inValue,
							const std::string& inName,
							const bool& inRequired,
							std::string& outValue) {
			if (inValue == nullptr) {
				NOTIFY_ERROR(h_, "GetStringValue: inValue == null");
				return false;
			}
			ValuePtr value = inValue->GetItem(inName);
			if (value == nullptr) {
				if (!inRequired) {
					outValue.clear();
					return true;
				}
				
				NOTIFY_ERROR(h_, "GetStringValue: value == null for required value");
				return false;
			}
			if (value->GetDataType() != DataType::kString) {
				NOTIFY_ERROR(h_, "GetStringValue: value->GetDataType() != DataType::kString");
				return false;
			}
			const StringValue& stringValue = static_cast<const StringValue&>(*value);
			outValue = stringValue.GetValue();
			return true;
		}
		
		//
		bool GetUInt64Value(const HermitPtr& h_,
							const ObjectValuePtr& inValue,
							const std::string& inName,
							const bool& inRequired,
							uint64_t& outValue) {
			if (inValue == nullptr) {
				NOTIFY_ERROR(h_, "GetUInt64Value: inValue == null");
				return false;
			}
			ValuePtr value = inValue->GetItem(inName);
			if (value == nullptr) {
				if (!inRequired) {
					outValue = 0;
					return true;
				}
				
				NOTIFY_ERROR(h_, "GetUInt64Value: value == null for required value");
				return false;
			}
			if (value->GetDataType() != DataType::kInt) {
				NOTIFY_ERROR(h_, "GetUInt64Value: value->GetDataType() != DataType::kInt");
				return false;
			}
			const IntValue& intValue = static_cast<const IntValue&>(*value);
			outValue = intValue.GetValue();
			return true;
		}
		
		//
		void OnSetItemError(const HermitPtr& h_, const std::string& inKey) {
			NOTIFY_ERROR(h_, "ObjectValueClassT::SetItem: inValue == null, inKey:", inKey);
		}
		
		//
		void OnEnumerateError(const HermitPtr& h_, const std::string& inKey) {
			NOTIFY_ERROR(h_, "ObjectValueClassT::EnumerateItems it->second == null, key:", inKey);
		}
		
	} // namespace value
} // namespace hermit
