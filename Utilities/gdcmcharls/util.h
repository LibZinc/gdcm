// 
// (C) Jan de Vaan 2007-2009, all rights reserved. See the accompanying "License.txt" for licensed use. 
// 


#ifndef CHARLS_UTIL
#define CHARLS_UTIL


#ifdef _DEBUG
#include <assert.h>
#define ASSERT(t) assert(t)
#else
#define ASSERT(t) {}
#endif

#if defined(WIN32)
#define CHARLS_IMEXPORT __declspec(dllexport) 

// default signed int types (32 or 64 bit)
#ifdef  _WIN64
typedef  int LONG;
#else
typedef int LONG;
#endif

#else
#include <stdint.h>

// default signed int types (32 or 64 bit)
typedef intptr_t LONG;
#endif



enum constants
{
  LONG_BITCOUNT = sizeof(LONG)*8
};

typedef unsigned char BYTE;
typedef unsigned short USHORT;


#include <string.h>
#include <stdlib.h>


#undef  NEAR

#ifdef _MSC_VER
#ifdef _DEBUG
#define inlinehint
#else
#define inlinehint __forceinline
#endif
#else
#define inlinehint __inline
#endif




#ifndef MAX
#define MAX(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef MIN
#define MIN(a,b)            (((a) < (b)) ? (a) : (b))
#endif


const LONG BASIC_RESET	= 64;

inline LONG log_2(LONG n)
{
	LONG x = 0;
	while (n > (LONG(1) << x))
	{
		++x;
	}
	return x;

}

struct Size
{
	Size(LONG width, LONG height) :
		cx(width),
		cy(height)
	{}
	LONG cx;
	LONG cy;
};



inline LONG Sign(LONG n)
	{ return (n >> (LONG_BITCOUNT-1)) | 1;}

inline LONG BitWiseSign(LONG i)
	{ return i >> (LONG_BITCOUNT-1); }	


#pragma pack(push, 1)

template<class SAMPLE>
struct Triplet
{ 
	Triplet() :
		v1(0),
		v2(0),
		v3(0)
	{};

	Triplet(LONG x1, LONG x2, LONG x3) :
		v1((SAMPLE)x1),
		v2((SAMPLE)x2),
		v3((SAMPLE)x3)
	{};

		union 
		{
			SAMPLE v1;
			SAMPLE R;
		};
		union 
		{ 
			SAMPLE v2;
			SAMPLE G;
		};
		union 
		{
			SAMPLE v3;
			SAMPLE B;
		};
};


#pragma pack(pop)


#include "interface.h"

inline bool operator==(const Triplet<BYTE>& lhs, const Triplet<BYTE>& rhs)
	{ return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3; }

inline bool  operator!=(const Triplet<BYTE>& lhs, const Triplet<BYTE>& rhs)
	{ return !(lhs == rhs); }

#endif
