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

#ifndef AsyncFunction_h
#define AsyncFunction_h

#include <memory>

//
#define DEFINE_ASYNC_FUNCTION_1A(NAME, A1) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_ENUMERATION_1A(NAME, A1) \
	class NAME { \
	public: \
		virtual bool Call(const A1& a1) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_2A(NAME, A1, A2) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, const A2& a2) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_3A(NAME, A1, A2, A3) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, const A2& a2, const A3& a3) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_4A(NAME, A1, A2, A3, A4) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, const A2& a2, const A3& a3, const A4& a4) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_5A(NAME, A1, A2, A3, A4, A5) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, const A2& a2, const A3& a3, const A4& a4, const A5& a5) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_6A(NAME, A1, A2, A3, A4, A5, A6) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, \
						  const A2& a2, \
						  const A3& a3, \
						  const A4& a4, \
						  const A5& a5, \
						  const A6& a6) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_7A(NAME, A1, A2, A3, A4, A5, A6, A7) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, \
						  const A2& a2, \
						  const A3& a3, \
						  const A4& a4, \
						  const A5& a5, \
						  const A6& a6, \
						  const A7& a7) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_8A(NAME, A1, A2, A3, A4, A5, A6, A7, A8) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, \
						  const A2& a2, \
						  const A3& a3, \
						  const A4& a4, \
						  const A5& a5, \
						  const A6& a6, \
						  const A7& a7, \
						  const A8& a8) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

//
#define DEFINE_ASYNC_FUNCTION_9A(NAME, A1, A2, A3, A4, A5, A6, A7, A8, A9) \
	class NAME { \
	public: \
		virtual void Call(const A1& a1, \
						  const A2& a2, \
						  const A3& a3, \
						  const A4& a4, \
						  const A5& a5, \
						  const A6& a6, \
						  const A7& a7, \
						  const A8& a8, \
						  const A9& a9) = 0; \
	}; \
	typedef std::shared_ptr<NAME> NAME##Ptr;

#endif
