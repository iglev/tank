#ifndef LIBBASE_EXECTOR_ASIOSVR_H_
#define LIBBASE_EXECTOR_ASIOSVR_H_

#ifdef _MSC_VER
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0601
#endif
#endif

#include "base/typedef.h"
#include <boost/exception/all.hpp>
#include <boost/noncopyable.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include <atomic>

NamespaceStart

class AsioSvc : boost::noncopyable
{
public:
	explicit AsioSvc(uint32 uPoolSize = 8);

	~AsioSvc() = default;

	void start();

	void stop();

	boost::asio::io_service& getSvc() {
		return mo_svc;
	}

	std::string getErrMsg() const {
		return ms_errMsg;
	}

	bool isRunning() { 
		return mb_isRunning; 
	}

	bool isPressCtrlC() {
		return mb_isCtrlC.load(std::memory_order_acquire);
	}

	void ctrlC(bool bVal) {
		mb_isCtrlC.store(bVal, std::memory_order_release);
	}

private:
	void run();

	void handleCtrlC(const boost::system::error_code& error, int signalNum);

private:
	std::atomic<bool> mb_isCtrlC;
	std::atomic<bool> mb_isRunning;
	std::atomic<bool> mb_hadPostLoop;
	uint32 mn_threadPoolSize;
	std::string ms_errMsg;

	boost::thread_group mo_threadMgr;
	boost::asio::io_service mo_svc;
	boost::asio::io_service::work mo_work;
	boost::asio::signal_set mo_signals;
};

NamespaceEnd


#endif
