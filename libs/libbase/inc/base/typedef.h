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

//��ȫɾ��ָ��
#define safeDelete(pData)		if(pData){ delete pData; pData = nullptr; }
#define safeDeleteArray(pData)	if(pData){ delete [] pData; pData = nullptr; }

//��(second)ת����΢��(microsecond)��1�� = 1000000΢��
#define Second2Microsecond(pn_second)				(uint64)(pn_second * 1000000)

//����(millisecond)ת����΢��(microsecond)��1���� = 1000΢��
#define Millisecond2Microsecond(pn_millisecond)	(uint64)(pn_millisecond * 1000)

//�����ռ�ͳһ����
#define NamespaceAuthor		tank
#define NamespaceUsing		using namespace tank;
#define NamespaceStart		namespace tank {
#define NamespaceEnd		}

NamespaceStart
NamespaceEnd

#endif

