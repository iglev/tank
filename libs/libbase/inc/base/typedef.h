#ifndef LIBBASE_COMMON_TYPEDEF_H_
#define LIBBASE_COMMON_TYPEDEF_H_

#include <boost/cstdint.hpp>

// unsigned
typedef boost::uint8_t ubyte;
typedef boost::uint8_t uint8;

//unsigned
typedef	boost::uint8_t			ubyte;
typedef boost::uint8_t			uint8;
typedef boost::uint16_t			uint16;
typedef boost::uint32_t			uint32;
typedef boost::uint64_t			uint64;
typedef boost::uintmax_t		uintmax;

//signed
typedef	boost::uint8_t			byte;
typedef boost::int8_t			int8;
typedef boost::int16_t			int16;
typedef boost::int32_t			int32;
typedef boost::int64_t			int64;
typedef boost::intmax_t			intmax;

#ifndef NULL
#define NULL					((void*)0)
#endif

//安全删除指针
#define safeDelete(pData)		if(pData){ delete pData; pData = nullptr; }
#define safeDeleteArray(pData)	if(pData){ delete [] pData; pData = nullptr; }

//秒(second)转化成微妙(microsecond)，1秒 = 1000000微妙
#define Second2Microsecond(pn_second)				(uint64)(pn_second * 1000000)

//毫秒(millisecond)转化成微妙(microsecond)，1毫秒 = 1000微妙
#define Millisecond2Microsecond(pn_millisecond)	(uint64)(pn_millisecond * 1000)

//命名空间统一定义
#define NamespaceAuthor		tank
#define NamespaceUsing		using namespace tank;
#define NamespaceStart		namespace tank {
#define NamespaceEnd		}

NamespaceStart
NamespaceEnd

#endif

