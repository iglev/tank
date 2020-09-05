#ifndef LIBBASE_COMMON_SAFE_SINGLETON_H_
#define LIBBASE_COMMON_SAFE_SINGLETON_H_

#include "base/typedef.h"
#include <mutex>

NamespaceStart

template<typename T>
class SafeSingleton
{
public:
	static T* prt() {
		if (mp_instance == nullptr) {
			std::lock_guard<std::mutex> lock(mo_mutex);
			if (mp_instance == nullptr) {
				volatile T* volatile tmp = new volatile T;
				mp_instance = tmp;
			}
		}
		return (T*)mp_instance;
	}

	static T& ref() {
		if (mp_instance == nullptr) {
			std::lock_guard<std::mutex> lock(mo_mutex);
			if (mp_instance == nullptr) {
				volatile T* volatile tmp = new volatile T;
				mp_instance = tmp;
			}
		}
		return *(T*)mp_instance;
	}

	static void release() {
		safeDelete(mp_instance);
	}

protected:
	SafeSingleton() {}
	virtual ~SafeSingleton() {}

private:
	static volatile T* volatile mp_instance;
	static std::mutex mo_mutex;
};

template<typename T>
volatile T* volatile SafeSingleton<T>::mp_instance = nullptr;

template<typename T>
std::mutex SafeSingleton<T>::mo_mutex;

NamespaceEnd

#endif
