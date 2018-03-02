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

#ifndef ParamBlock_h
#define ParamBlock_h

//
//
#define DEFINE_PARAMBLOCK_3( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
};

//
//
#define DEFINE_PARAMBLOCK_4( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
};

//
//
#define DEFINE_PARAMBLOCK_5( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
};

//
//
#define DEFINE_PARAMBLOCK_6( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
};

//
//
#define DEFINE_PARAMBLOCK_8( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
};

//
//
#define DEFINE_PARAMBLOCK_9( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
};

//
//
#define DEFINE_PARAMBLOCK_10( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name, \
	P10Type, P10Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9, \
		const P10Type& inP10) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9), \
		P10Name(inP10) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
	P10Type P10Name; \
};

//
//
#define DEFINE_PARAMBLOCK_11( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name, \
	P10Type, P10Name, \
	P11Type, P11Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9, \
		const P10Type& inP10, \
		const P11Type& inP11) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9), \
		P10Name(inP10), \
		P11Name(inP11) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
	P10Type P10Name; \
	P11Type P11Name; \
};

//
//
#define DEFINE_PARAMBLOCK_12( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name, \
	P10Type, P10Name, \
	P11Type, P11Name, \
	P12Type, P12Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9, \
		const P10Type& inP10, \
		const P11Type& inP11, \
		const P12Type& inP12) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9), \
		P10Name(inP10), \
		P11Name(inP11), \
		P12Name(inP12) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
	P10Type P10Name; \
	P11Type P11Name; \
	P12Type P12Name; \
};

//
//
#define DEFINE_PARAMBLOCK_13( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name, \
	P10Type, P10Name, \
	P11Type, P11Name, \
	P12Type, P12Name, \
	P13Type, P13Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9, \
		const P10Type& inP10, \
		const P11Type& inP11, \
		const P12Type& inP12, \
		const P13Type& inP13) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9), \
		P10Name(inP10), \
		P11Name(inP11), \
		P12Name(inP12), \
		P13Name(inP13) \
	{ \
	} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
	P10Type P10Name; \
	P11Type P11Name; \
	P12Type P12Name; \
	P13Type P13Name; \
};

//
//
#define DEFINE_PARAMBLOCK_14( \
	Name, \
	P1Type, P1Name, \
	P2Type, P2Name, \
	P3Type, P3Name, \
	P4Type, P4Name, \
	P5Type, P5Name, \
	P6Type, P6Name, \
	P7Type, P7Name, \
	P8Type, P8Name, \
	P9Type, P9Name, \
	P10Type, P10Name, \
	P11Type, P11Name, \
	P12Type, P12Name, \
	P13Type, P13Name, \
	P14Type, P14Name) \
struct Name { \
	Name( \
		const P1Type& inP1, \
		const P2Type& inP2, \
		const P3Type& inP3, \
		const P4Type& inP4, \
		const P5Type& inP5, \
		const P6Type& inP6, \
		const P7Type& inP7, \
		const P8Type& inP8, \
		const P9Type& inP9, \
		const P10Type& inP10, \
		const P11Type& inP11, \
		const P12Type& inP12, \
		const P13Type& inP13, \
		const P14Type& inP14) \
		: \
		P1Name(inP1), \
		P2Name(inP2), \
		P3Name(inP3), \
		P4Name(inP4), \
		P5Name(inP5), \
		P6Name(inP6), \
		P7Name(inP7), \
		P8Name(inP8), \
		P9Name(inP9), \
		P10Name(inP10), \
		P11Name(inP11), \
		P12Name(inP12), \
		P13Name(inP13), \
		P14Name(inP14) \
		{ \
		} \
	P1Type P1Name; \
	P2Type P2Name; \
	P3Type P3Name; \
	P4Type P4Name; \
	P5Type P5Name; \
	P6Type P6Name; \
	P7Type P7Name; \
	P8Type P8Name; \
	P9Type P9Name; \
	P10Type P10Name; \
	P11Type P11Name; \
	P12Type P12Name; \
	P13Type P13Name; \
	P14Type P14Name; \
};

#endif
