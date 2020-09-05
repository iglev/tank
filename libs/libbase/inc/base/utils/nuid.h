#ifndef LIBBASE_NUID_H_
#define LIBBASE_NUID_H_

#include "base/typedef.h"
#include <boost/noncopyable.hpp>
#include <ctime>

NamespaceStart

template<typename T, uint32 BIT>
class Nuid : boost::noncopyable
{
public:
	explicit Nuid()
		: mn_currTime(std::time(nullptr))
		, mn_startTime(mn_currTime-1)
		, mn_limit(0)
		, mn_count(0)
	{

	}

	T nextID()
	{
		std::time_t uCurr = std::time(nullptr);
		if (mn_limit > ((1 << BIT) - 1))
		{
			if (uCurr == mn_currTime) {
				return 0;
			}
			mn_currTime = uCurr;
			mn_limit = 0;
		}
		++mn_limit;

		T uDiff = (T)(uCurr - mn_startTime);
		mn_count = (mn_count + 1) % (1 << BIT);
		return ((uDiff << BIT)) | ((T)mn_count);
	}

private:
	std::time_t mn_currTime;
	std::time_t mn_startTime;
	T mn_limit;
	T mn_count;
};

typedef Nuid<uint32, 7> Nuid32Year128;
typedef Nuid<uint32, 10> Nuid32Month1024;
typedef Nuid<uint32, 12> Nuid32Weak4096;
typedef Nuid<uint64, 12> Nuid64Never4096;

NamespaceEnd

#endif

