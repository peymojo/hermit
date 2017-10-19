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

#ifndef Callback_h
#define Callback_h

//
//
#define DEFINE_CALLBACK_1A(NAME, A1) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref() \
			: \
			mCallback(0) \
			{ \
			} \
		\
		NAME##Ref( \
			NAME* inCallback) \
			: \
			mCallback(inCallback) \
			{ \
			} \
		\
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(&inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1) const \
		{ \
			if (mCallback == 0) \
			{ \
				return false; \
			} \
			return mCallback->Function(a1); \
		} \
		\
		bool IsNull() const \
		{ \
			return (mCallback == 0); \
		} \
		\
		NAME* mCallback; \
	};

//
//
#define DEFINE_CALLBACK_2A(NAME, A1, A2) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		virtual bool Function( \
			const A1& a1, \
			const A2& a2) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref() \
			: \
			mCallback(0) \
		{ \
		} \
		\
		NAME##Ref( \
			NAME* inCallback) \
			: \
			mCallback(inCallback) \
		{ \
		} \
		\
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(&inCallback) \
		{ \
		} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2) const \
		{ \
			if (mCallback == 0) \
			{ \
				return false; \
			} \
			return mCallback->Function(a1, a2); \
		} \
		\
		bool IsNull() const \
		{ \
			return (mCallback == 0); \
		} \
		\
		NAME* mCallback; \
	};
	
//
//
#define DEFINE_CALLBACK_3A(NAME, A1, A2, A3) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref() \
			: \
			mCallback(0) \
			{ \
			} \
		\
		NAME##Ref( \
			NAME* inCallback) \
			: \
			mCallback(inCallback) \
		{ \
		} \
		\
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(&inCallback) \
		{ \
		} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3) const \
		{ \
			return mCallback->Function(a1, a2, a3); \
		} \
		\
		bool IsNull() const \
		{ \
			return (mCallback == 0); \
		} \
		\
		NAME* mCallback; \
	};
				
//
//
#define DEFINE_CALLBACK_4A(NAME, A1, A2, A3, A4) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref() \
			: \
			mCallback(0) \
			{ \
			} \
		\
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(&inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4) const \
		{ \
			return mCallback->Function(a1, a2, a3, a4); \
		} \
		\
		bool IsNull() const \
		{ \
			return (mCallback == 0); \
		} \
		\
		NAME* mCallback; \
	};

//
//
#define DEFINE_CALLBACK_5A(NAME, A1, A2, A3, A4, A5) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(&inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5) const \
		{ \
			return mCallback->Function(a1, a2, a3, a4, a5); \
		} \
		\
		bool IsNull() const \
		{ \
			return (mCallback == 0); \
		} \
		\
		NAME* mCallback; \
	};

//
//
#define DEFINE_CALLBACK_6A(NAME, A1, A2, A3, A4, A5, A6) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6) const \
		{ \
			return mCallback.Function(a1, a2, a3, a4, a5, a6); \
		} \
		\
		NAME& mCallback; \
	};

//
//
#define DEFINE_CALLBACK_7A(NAME, A1, A2, A3, A4, A5, A6, A7) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7) const \
		{ \
			return mCallback.Function(a1, a2, a3, a4, a5, a6, a7); \
		} \
		\
		NAME& mCallback; \
	};

//
//
#define DEFINE_CALLBACK_8A(NAME, A1, A2, A3, A4, A5, A6, A7, A8) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7, \
			const A8& a8) = 0; \
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7, \
			const A8& a8) const \
		{ \
			return mCallback.Function(a1, a2, a3, a4, a5, a6, a7, a8); \
		} \
		\
		NAME& mCallback; \
	};

//
//
#define DEFINE_CALLBACK_9A(NAME, A1, A2, A3, A4, A5, A6, A7, A8, A9) \
	class NAME \
	{ \
	public: \
		NAME() \
		{ \
		} \
		\
		virtual bool Function( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7, \
			const A8& a8, \
			const A9& a9) = 0; \
		\
	}; \
	\
	class NAME##Ref \
	{ \
	public: \
		NAME##Ref( \
			NAME& inCallback) \
			: \
			mCallback(inCallback) \
			{ \
			} \
		\
		bool Call( \
			const A1& a1, \
			const A2& a2, \
			const A3& a3, \
			const A4& a4, \
			const A5& a5, \
			const A6& a6, \
			const A7& a7, \
			const A8& a8, \
			const A9& a9) const \
		{ \
			return mCallback.Function(a1, a2, a3, a4, a5, a6, a7, a8, a9); \
		} \
		\
		NAME& mCallback; \
	};

#endif

