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

#ifndef Value_h
#define Value_h

#include <stddef.h>
#include <string>
#include "Hermit/Foundation/Hermit.h"
#include "EnumerateDataValuesFunction.h"

namespace hermit {
	namespace value {

		//
		class Value {
		public:
			//
			virtual DataType GetDataType() const = 0;
		};
		typedef std::shared_ptr<Value> ValuePtr;
		
		//
		bool EnumerateOneValue(const HermitPtr& h_,
							   const std::string& name,
							   const Value& value,
							   EnumerateDataValuesCallback& callback);
		
		//
		class IntValue : public Value {
		public:
			//
			static ValuePtr New(int64_t inValue) {
				return std::make_shared<IntValue>(inValue);
			}
			
			//
			IntValue() : mValue(0) {
			}
			
			//
			IntValue(int64_t inValue) : mValue(inValue) {
			}
			
			//
			virtual DataType GetDataType() const {
				return DataType::kInt;
			}
			
			//
			int64_t GetValue() const {
				return mValue;
			}
			
			//
			int64_t mValue;
		};
		
		//
		class BoolValue : public Value {
		public:
			//
			static ValuePtr New(bool inValue) {
				return std::make_shared<BoolValue>(inValue);
			}

			//
			BoolValue() : mValue(false) {
			}
			
			//
			BoolValue(bool inValue) : mValue(inValue) {
			}
			
			//
			virtual DataType GetDataType() const {
				return DataType::kBool;
			}
			
			//
			bool GetValue() const {
				return mValue;
			}
			
			//
			bool mValue;
		};
		
		//
		class StringValue : public Value {
		public:
			//
			static ValuePtr New(const char* inValue) {
				return std::make_shared<StringValue>(inValue);
			}
			
			//
			static ValuePtr New(const std::string& inValue) {
				return std::make_shared<StringValue>(inValue);
			}
			
			//
			StringValue() {
			}
			
			//
			StringValue(const char* inValue) : mValue(inValue) {
			}
			
			//
			StringValue(const std::string& inValue) : mValue(inValue) {
			}
			
			//
			virtual DataType GetDataType() const {
				return DataType::kString;
			}

			//
			virtual const std::string& GetValue() const {
				return mValue;
			}
			
			//
			std::string mValue;
		};
		typedef std::shared_ptr<StringValue> StringValuePtr;
		
		//
		class ObjectValueVisitor {
		public:
			//
			virtual bool VisitValue(const HermitPtr& h_, const std::string& inKey, const Value& inValue) = 0;
		};
		
		//
		class ObjectValue : public Value {
		public:
			//
			ObjectValue() {
			}
			
			//
			virtual DataType GetDataType() const {
				return DataType::kObject;
			}
			
			//
			virtual size_t GetItemCount() const = 0;
			
			//
			virtual ValuePtr GetItem(const std::string& inKey) const = 0;
			
			//
			virtual	bool EnumerateItems(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const = 0;
			
			//
			virtual bool VisitItems(const HermitPtr& h_, ObjectValueVisitor& inVisitor) const = 0;
			
			//
			virtual void SetItem(const HermitPtr& h_, const std::string& inKey, const ValuePtr& inValue) = 0;
			
			//
			virtual void DeleteItem(const std::string& inKey) = 0;
		};
		typedef std::shared_ptr<ObjectValue> ObjectValuePtr;
		
		//
		void OnEnumerateError(const HermitPtr& h_, const std::string& inKey);
		
		//
		void OnSetItemError(const HermitPtr& h_, const std::string& inKey);
		
		//
		template <typename ValueMapT>
		class ObjectValueClassT : public ObjectValue {
		public:
			//
			static ValuePtr New() {
				return std::make_shared<ObjectValueClassT<ValueMapT>>();
			}

			//
			static ValuePtr New(const ValueMapT& inValues) {
				return std::make_shared<ObjectValueClassT<ValueMapT>>(inValues);
			}

			//
			ObjectValueClassT() {
			}
			
			//
			ObjectValueClassT(const ValueMapT& inValues) : mValues(inValues) {
			}
			
			//
			virtual size_t GetItemCount() const override {
				return mValues.size();
			}
			
			//
			virtual ValuePtr GetItem(const std::string& inKey) const override {
				auto it = mValues.find(inKey);
				if (it == mValues.end()) {
					return ValuePtr();
				}
				return it->second;
			}
			
			//
			virtual	bool EnumerateItems(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override {
				auto end = mValues.end();
				for (auto it = mValues.begin(); it != end; ++it) {
					if (it->second == nullptr) {
						OnEnumerateError(h_, it->first);
						return false;
					}
					if (!EnumerateOneValue(h_, it->first, *it->second, inCallback)) {
						return false;
					}
				}
				return true;
			}
			
			//
			virtual bool VisitItems(const HermitPtr& h_, ObjectValueVisitor& inVisitor) const override {
				auto end = mValues.end();
				for (auto it = mValues.begin(); it != end; ++it) {
					if (!inVisitor.VisitValue(h_, it->first, *it->second)) {
						return false;
					}
				}
				return true;
			}
			
			//
			virtual void SetItem(const HermitPtr& h_, const std::string& inKey, const ValuePtr& inValue) override {
				if (inValue == nullptr) {
					OnSetItemError(h_, inKey);
					return;
				}
				
				auto it = mValues.find(inKey);
				if (it == mValues.end()) {
					mValues.insert(typename ValueMapT::value_type(inKey, inValue));
				}
				else {
					it->second = inValue;
				}
			}
			
			//
			virtual void DeleteItem(const std::string& inKey) override {
				mValues.erase(inKey);
			}
			
			//
			//
			ValueMapT mValues;
		};
		
		//
		class ArrayValueVisitor
		{
		public:
			//
			virtual bool VisitValue(const HermitPtr& h_, const Value& inValue) = 0;
		};
		
		//
		class ArrayValuePtrVisitor
		{
		public:
			//
			virtual bool VisitValue(const HermitPtr& h_, const ValuePtr& inValue) = 0;
		};
		
		//
		class ArrayValue : public Value {
		public:
			//
			ArrayValue() = default;
			
			//
			virtual ~ArrayValue() = default;
			
			//
			virtual DataType GetDataType() const {
				return DataType::kArray;
			}
			
			//
			virtual size_t GetItemCount() const = 0;
			
			//
			virtual ValuePtr GetItem(size_t inIndex) const = 0;
			
			//
			virtual bool VisitItems(const HermitPtr& h_, ArrayValueVisitor& inVisitor) const = 0;
			
			//
			virtual bool VisitItemPtrs(const HermitPtr& h_, ArrayValuePtrVisitor& inVisitor) const = 0;
			
			//
			virtual	bool EnumerateItems(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const = 0;
			
			//
			virtual void AppendItem(const ValuePtr& inValue) = 0;
		};
		typedef std::shared_ptr<ArrayValue> ArrayValuePtr;
		
		//
		template <typename ValueVectorT>
		class ArrayValueClassT : public ArrayValue {
		public:
			//
			static ValuePtr New() {
				return std::make_shared<ArrayValueClassT<ValueVectorT>>();
			}
			
			//
			static ValuePtr New(const ValueVectorT& inValues) {
				return std::make_shared<ArrayValueClassT<ValueVectorT>>(inValues);
			}

			//
			ArrayValueClassT() {
			}
			
			//
			ArrayValueClassT(const ValueVectorT& inItems) : mItems(inItems) {
			}
			
			//
			virtual size_t GetItemCount() const override {
				return mItems.size();
			}
			
			//
			virtual ValuePtr GetItem(size_t inIndex) const override {
				if (inIndex >= mItems.size()) {
					return ValuePtr();
				}
				return mItems[inIndex];
			}
			
			//
			virtual bool VisitItems(const HermitPtr& h_, ArrayValueVisitor& inVisitor) const override {
				auto end = mItems.end();
				for (auto it = mItems.begin(); it != end; ++it) {
					if (!inVisitor.VisitValue(h_, **it)) {
						return false;
					}
				}
				return true;
			}
			
			//
			virtual bool VisitItemPtrs(const HermitPtr& h_, ArrayValuePtrVisitor& inVisitor) const override {
				auto end = mItems.end();
				for (auto it = mItems.begin(); it != end; ++it) {
					if (!inVisitor.VisitValue(h_, *it)) {
						return false;
					}
				}
				return true;
			}
			
			//
			virtual	bool EnumerateItems(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override {
				auto end = mItems.end();
				for (auto it = mItems.begin(); it != end; ++it) {
					if (!EnumerateOneValue(h_, "", **it, inCallback)) {
						return false;
					}
				}
				return true;
			}
			
			//
			virtual void AppendItem(const ValuePtr& inValue) override {
				mItems.push_back(inValue);
			}
			
			//
			ValueVectorT mItems;
		};
		
		//
		class EnumerateArrayValuesFunction : public EnumerateDataValuesFunction {
		public:
			//
			EnumerateArrayValuesFunction(const ArrayValue& inArrayValue) : mArrayValue(inArrayValue) {
			}
			
			//
			virtual bool Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override {
				return mArrayValue.EnumerateItems(h_, inCallback);
			}
			
			//
			const ArrayValue& mArrayValue;
		};
		
		//
		class EnumerateObjectValuesFunction : public EnumerateDataValuesFunction {
		public:
			//
			EnumerateObjectValuesFunction(const ObjectValue& inObjectValue) : mObjectValue(inObjectValue) {
			}
			
			//
			virtual bool Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override {
				return mObjectValue.EnumerateItems(h_, inCallback);
			}
			
			//
			const ObjectValue& mObjectValue;
		};
		
		//
		class EnumerateValuesFunction : public EnumerateDataValuesFunction {
		public:
			//
			EnumerateValuesFunction(const Value& inValue);
			
			//
			virtual bool Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override;
			
			//
			const Value& mValue;
		};
		
		//
		class EnumerateValuesFunctionClass : public EnumerateDataValuesFunction {
		public:
			//
			EnumerateValuesFunctionClass(const ValuePtr& inValue);
			
			//
			virtual bool Function(const HermitPtr& h_, EnumerateDataValuesCallback& inCallback) const override;
			
			//
			ValuePtr mValue;
		};
		
		//
		class EnumerateValuesCallback : public EnumerateDataValuesCallback {
		public:
			//
			EnumerateValuesCallback(Value& inValue) : mValue(inValue) {
			}
			
			//
			bool AddValue(const HermitPtr& h_, const std::string& inName, const ValuePtr& inValue);
			
			//
			Value& mValue;
		};
		
		//
		template <typename StringT, typename ValueMapT, typename ValueVectorT>
		class EnumerateValuesCallbackT : public EnumerateValuesCallback {
		public:
			//
			EnumerateValuesCallbackT(Value& inValue) :
				EnumerateValuesCallback(inValue),
				mSuccess(false) {
			}
			
			//
			virtual bool Function(const HermitPtr& h_,
								  bool inSuccess,
								  const std::string& inName,
								  const DataType& inType,
								  const void* inValue) override {
				mSuccess = inSuccess;
				if (!inSuccess) {
					return false;
				}
				
				ValuePtr value;
				if (inType == DataType::kInt) {
					value = IntValue::New(*(const int64_t*)inValue);
				}
				else if (inType == DataType::kBool) {
					value = BoolValue::New(*(const bool*)inValue);
				}
				else if (inType == DataType::kString) {
					value = StringValue::New((const char*)inValue);
				}
				else if (inType == DataType::kArray) {
					value = ArrayValueClassT<ValueVectorT>::New();
					EnumerateDataValuesFunction* f = (EnumerateDataValuesFunction*)inValue;
					EnumerateValuesCallbackT callback(*value);
					if (!f->Call(h_, callback)) {
						if (!callback.mSuccess) {
							mSuccess = false;
						}
						return false;
					}
				}
				else if (inType == DataType::kObject) {
					value = ObjectValueClassT<ValueMapT>::New();
					EnumerateDataValuesFunction* f = (EnumerateDataValuesFunction*)inValue;
					EnumerateValuesCallbackT callback(*value);
					if (!f->Call(h_, callback)) {
						if (!callback.mSuccess) {
							mSuccess = false;
						}
						return false;
					}
				}
				if (!AddValue(h_, inName, value)) {
					mSuccess = false;
					return false;
				}
				return true;
			}
			
			//
			bool mSuccess;
		};
		
		//
		ValuePtr DuplicateValue(const HermitPtr& h_, const Value& inValue);
		
		//
		bool GetStringValue(const HermitPtr& h_,
							const ObjectValuePtr& inValue,
							const std::string& inName,
							const bool& inRequired,
							std::string& outValue);
		
		//
		bool GetUInt64Value(const HermitPtr& h_,
							const ObjectValuePtr& inValue,
							const std::string& inName,
							const bool& inRequired,
							uint64_t& outValue);
	
	} // namespace value
} // namespace hermit

#endif
