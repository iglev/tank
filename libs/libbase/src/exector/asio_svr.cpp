#include "base/exector/asio_svr.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"

#include <iostream>

NamespaceUsing

///////////////////////////////////////////////////////////////////////////
// public

AsioSvc::AsioSvc(uint32 uPoolSize)
	: mb_isCtrlC(false)
	, mb_isRunning(false)
	, mn_threadPoolSize(uPoolSize)
	, mo_svc(uPoolSize > 0 ? uPoolSize : 1)
	, mo_work(mo_svc)
	, mo_signals(mo_svc)
{
	mo_signals.add(SIGINT);
	mo_signals.add(SIGTERM);
#ifdef SIGQUIT
	mo_signals.add(SIGQUIT);
#endif
	mo_signals.async_wait(boost::bind(&AsioSvc::handleCtrlC, this, boost::placeholders::_1, boost::placeholders::_2));

}

void AsioSvc::start()
{
	if (mb_isRunning) {
		return;
	}

	mb_isRunning = true;
	for (uint32 i = 0; i < mn_threadPoolSize; ++i) {
		mo_threadMgr.create_thread(boost::bind(&AsioSvc::run, this));
	}
;}

void AsioSvc::stop()
{
	if (!mo_svc.stopped()) {
		mo_svc.stop();
	}
	mo_threadMgr.join_all();
	mb_isRunning = false;
}


//////////////////////////////////////////////////////////////////////////
// private

void AsioSvc::run()
{
	while (true) {
		try {
			mo_svc.run();
			break;
		}
		catch (std::exception& e) {
			LOGINFO(TI_LIBBASE, "AsioSvc::run, std::exception:", e.what());
		}
		catch (...) {
			LOGINFO(TI_LIBBASE, "AsioSvc::run, other exception");
		}
	}
}

void AsioSvc::handleCtrlC(const boost::system::error_code &error, int signalNum)
{
	mb_isCtrlC.store(true, std::memory_order_release);
}





