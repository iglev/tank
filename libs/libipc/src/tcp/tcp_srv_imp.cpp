#include "tcp/tcp_srv_imp.h"

NamespaceUsing

TcpSrvImp::TcpSrvImp(TcpSrvActor* pSrvActor)
	: mp_srvactor(pSrvActor)
	, mp_acceptor(nullptr)
	, mp_timer(nullptr)
	, mb_accepting(false)
	, mb_closing(false)
	, mn_srvid(0)
	, mn_maxLimit(0)
	, mn_checkTime(0)
	, mn_timeout(0)
{

}

TcpSrvImp::~TcpSrvImp()
{
	safeDelete(mp_acceptor);
	safeDelete(mp_timer);
}

///////////////////////////////////////////////////////////////////////////////
// req

void TcpSrvImp::reqShutdown(const AmsgPtr& pMsg)
{
	if (mb_closing)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqShutdown, closing");
		return;
	}
	mb_closing = true;

	if (mp_acceptor)
	{
		boost::system::error_code error;
		mp_acceptor->cancel(error);
		mp_acceptor->close(error);
	}
	closeAllSession("TcpSrvImp::reqShutdown", true);
}

void TcpSrvImp::reqClose(const AmsgPtr& pMsg)
{
	TcpSCloseAmsg* pCloseMsg = (TcpSCloseAmsg*)(pMsg.get());
	if (pCloseMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqClose srvid", pCloseMsg->mn_srvid, mn_srvid);
		return;
	}

	ActorPtr pSession = mp_srvactor->getActor(pCloseMsg->ms_sessionAddr);
	if (!pSession)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqClose, not found session", pCloseMsg->ms_sessionAddr);
		return;
	}

	ACTOR_CAST(TcpSession, pSession)->postClose(false, "TcpSrvImp::reqClose", false);
}

void TcpSrvImp::reqCloseSafe(const AmsgPtr& pMsg)
{
	TcpSCloseSafeAmsg* pCloseMsg = (TcpSCloseSafeAmsg*)(pMsg.get());
	if (pCloseMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqCloseSafe srvid", pCloseMsg->mn_srvid, mn_srvid);
		return;
	}

	ActorPtr pSession = mp_srvactor->getActor(pCloseMsg->ms_sessionAddr);
	if (!pSession)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqCloseSafe, not found session", pCloseMsg->ms_sessionAddr);
		return;
	}

	ACTOR_CAST(TcpSession, pSession)->postClose(false, "TcpSrvImp::reqClose", true);
}

void TcpSrvImp::reqCloseAll(const AmsgPtr& pMsg)
{
	TcpSCloseAllAmsg* pCloseAllMsg = (TcpSCloseAllAmsg*)(pMsg.get());
	if (pCloseAllMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqCloseAll, srvid", pCloseAllMsg, mn_srvid);
		return;
	}
	closeAllSession("TcpSrvImp::reqCloseAll", false);
}

void TcpSrvImp::reqCloseAllSafe(const AmsgPtr& pMsg)
{
	TcpSCloseAllSafeAmsg* pCloseAllMsg = (TcpSCloseAllSafeAmsg*)(pMsg.get());
	if (pCloseAllMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqCloseAllSafe, srvid", pCloseAllMsg, mn_srvid);
		return;
	}
	closeAllSession("TcpSrvImp::reqCloseAllSafe", true);
}

void TcpSrvImp::reqSetTesting(const AmsgPtr& pMsg)
{
	TcpSSettestAmsg* pSetTestingMsg = (TcpSSettestAmsg*)(pMsg.get());
	if (pSetTestingMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqSetTesting, srvid", pSetTestingMsg->mn_srvid, mn_srvid);
		return;
	}

	auto iter = mo_mapConnection.find(pSetTestingMsg->ms_sessionAddr);
	if (iter == mo_mapConnection.end())
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqSetTesting, not found session", pSetTestingMsg->ms_sessionAddr);
		return;
	}

	ActorPtr pSession = std::get<1>(iter->second);
	ACTOR_CAST(TcpSession, pSession)->postSetTest(pSetTestingMsg->mb_pass);

	mo_mapConnection.erase(iter);
	mo_mapTested.insert(std::make_pair(pSession->getAddr(), std::time(nullptr)));
}

void TcpSrvImp::reqSetActor(const AmsgPtr& pMsg)
{
	TcpSSetActorAmsg* pSetActorMsg = (TcpSSetActorAmsg*)pMsg.get();
	if (pSetActorMsg->mn_srvid != mn_srvid)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqSetActor. srvid", pSetActorMsg->mn_srvid, mn_srvid);
		return;
	}

	auto iter = mo_mapTested.find(pSetActorMsg->ms_sessionAddr);
	if (iter == mo_mapTested.end())
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::reqSetActor, not found session", pSetActorMsg->ms_sessionAddr);
		return;
	}

	ActorPtr pSession = mp_srvactor->getActor(iter->first);
	if (pSession)
	{
		ACTOR_CAST(TcpSession, pSession)->postSetActor(pSetActorMsg->mp_actor);
	}
}

///////////////////////////////////////////////////////////////////////////////
// listen

bool TcpSrvImp::onListen(const std::string& strRoot, uint32 uSrvID, const std::string& strIP,
	uint16 uPort, uint32 uMaxLimit, uint32 uCheckTime, uint32 uTimeout)
{
	if (uSrvID <= 0 || strIP.empty() || uPort <= 0
		|| uMaxLimit <= 0 || uCheckTime <= 0 || uTimeout <= 0
		|| mp_timer || mp_acceptor || mb_accepting)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onListen, args error",
			strRoot, uSrvID, strIP, uPort, uMaxLimit, uCheckTime, uTimeout);
		return false;
	}

	ms_root = strRoot;
	mn_srvid = uSrvID;
	mn_maxLimit = uMaxLimit;
	mn_checkTime = uCheckTime;
	mn_timeout = uTimeout;
	mo_endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(strIP), uPort);

	try
	{
		mp_timer = new boost::asio::steady_timer(
			mp_srvactor->getSvr()->getSvc(),
			std::chrono::nanoseconds(mn_checkTime * 1000000000LL)
		);
		mp_acceptor = new boost::asio::ip::tcp::acceptor(mp_srvactor->getSvr()->getSvc());
	}
	catch (std::exception& e)
	{
		safeDelete(mp_timer);
		safeDelete(mp_acceptor);
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onListen, new timer acceptor fail", e.what());
		return false;
	}

	// acceptor start listen
	try
	{
		mp_acceptor->open(mo_endpoint.protocol());
		mp_acceptor->set_option(boost::asio::ip::tcp::acceptor::reuse_address(true));
		mp_acceptor->bind(mo_endpoint);
		mp_acceptor->listen(mn_maxLimit);
	}
	catch (boost::system::system_error& e)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onListen, start accept fail", e.what());
		return false;
	}

	// start timer
	try
	{
		mp_timer->expires_from_now(std::chrono::nanoseconds(mn_checkTime * 1000000000LL));
		mp_timer->async_wait(boost::bind(&TcpSrvImp::doExpired, this, boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onListen, start timer", e.what());
		return false;
	}

	// start accept
	mp_srvactor->getChannel().post(boost::bind(&TcpSrvImp::onAccept, this));

	// cb
	cbOnSrvStartup();
	return true;
}

///////////////////////////////////////////////////////////////////////////////
// accept

void TcpSrvImp::onAccept()
{
	if (mb_accepting || mb_closing)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onAccept, closing or accepting", mb_accepting.load(), mb_closing.load());
		return;
	}

	mb_accepting = true;
	uint32 uSid = mo_id.nextID();
	if (uSid == 0)
	{
		// too many, overload
		ActorPtr pSession = getNewSession(mp_srvactor, 0, 0);
		mp_acceptor->async_accept(ACTOR_CAST(TcpSession, pSession)->getSocket(),
			boost::bind(&TcpSrvImp::doAcceptedFail, this, boost::asio::placeholders::error, pSession));
	}
	else
	{
		ActorPtr pSession = getNewSession(mp_srvactor, mn_srvid, uSid);
		mp_acceptor->async_accept(ACTOR_CAST(TcpSession, pSession)->getSocket(),
			boost::bind(&TcpSrvImp::doAccepted, this, boost::asio::placeholders::error, pSession));
	}
}

// accepted fail
void TcpSrvImp::doAcceptedFail(const boost::system::error_code& error, ActorPtr& pSession)
{
	if (error)
	{
		// only log
		LOGINFO(TI_LIBIPC, "TcpSrvImp::doAcceptedFail", error.message());
	}
	// must free session
	if (pSession)
	{
		boost::system::error_code error;
		((TcpSession*)pSession.get())->getSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		((TcpSession*)pSession.get())->getSocket().close(error);
		pSession.reset();
	}
	mb_accepting = false;
}

// accepted success
void TcpSrvImp::doAccepted(const boost::system::error_code& error, ActorPtr& pSession)
{
	mb_accepting = false;
	if (error)
	{
		boost::system::error_code error;
		auto tmp = ACTOR_CAST(TcpSession, pSession);
		tmp->getSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		tmp->getSocket().close(error);
		pSession.reset();
		return;
	}

	if (pSession)
	{
		mp_srvactor->getChannel().post(boost::bind(&TcpSrvImp::onAccepted, this, pSession));
	}
	mp_srvactor->getChannel().post(boost::bind(&TcpSrvImp::onAccept, this));
}

// accepted
void TcpSrvImp::onAccepted(ActorPtr& pSession)
{
	if (mb_closing)
	{
		// closing, close curr session
		boost::system::error_code error;
		auto tmp = ACTOR_CAST(TcpSession, pSession);
		tmp->getSocket().shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		tmp->getSocket().close(error);
		pSession.reset();
		return;
	}

	mp_srvactor->addActor(pSession);
	pSession->addActor(pSession);
	mo_mapConnection.insert(std::make_pair(pSession->getAddr(), std::make_tuple(std::time(nullptr), pSession)));
	ACTOR_CAST(TcpSession, pSession)->startRecv();
}

void TcpSrvImp::doExpired(const boost::system::error_code& err)
{
	mp_srvactor->getChannel().post(boost::bind(&TcpSrvImp::onExpired, this));
}

void TcpSrvImp::onExpired()
{
	if (mb_closing)
	{
		// check kill self
		if (mo_mapConnection.size() == 0 && mo_mapTimeoutConnection.size() == 0 && mo_mapTested.size() == 0)
		{
			// cancel, whatever
			boost::system::error_code error;
			mp_timer->cancel(error);

			// cb shutdown
			cbOnSrvShutdown();

			// notify root kill res
			ACTOR_SEND_NEW_MSG(mp_srvactor, ms_root, ShutdownResAmsg, ());

			// kill self
			ACTOR_SEND_NEW_MSG(mp_srvactor, mp_srvactor->getAddr(), KillSelfAmsg, (mp_srvactor->getActor(mp_srvactor->getAddr())));
			return;
		}
	}

	// check timeout, not test
	std::time_t uCurrTime = std::time(nullptr);
	for (auto iter = mo_mapConnection.begin(); iter != mo_mapConnection.end();)
	{
		std::time_t uTime = std::get<0>(iter->second);
		if ((uCurrTime - uTime) >= mn_timeout)
		{
			ActorPtr pSession = std::get<1>(iter->second);
			ACTOR_CAST(TcpSession, pSession)->postClose(false, "TcpSrvImp::onExpired, timeout tested", false);
			iter = mo_mapConnection.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	if (!mb_accepting)
	{
		mp_srvactor->getChannel().post(boost::bind(&TcpSrvImp::onAccept, this));
	}

	try
	{
		mp_timer->expires_from_now(std::chrono::nanoseconds(mn_checkTime * 1000000000LL));
		mp_timer->async_wait(boost::bind(&TcpSrvImp::doExpired, this, boost::asio::placeholders::error));
	}
	catch (boost::system::system_error& e)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::onExpired, start timer", e.what());
	}
}

///////////////////////////////////////////////////////////////////////////////
// cb

void TcpSrvImp::cbOnSrvStartup()
{
	ACTOR_SEND_NEW_MSG(mp_srvactor, ms_root, TcpSStartUpCbAmsg, (mn_srvid));
}

void TcpSrvImp::cbOnSrvShutdown()
{
	ACTOR_SEND_NEW_MSG(mp_srvactor, ms_root, TcpSShutdownCbAmsg, (mn_srvid));
}

void TcpSrvImp::cbOnSrvConnected(const AmsgPtr& pMsg)
{
	ACTOR_SEND_MSG(mp_srvactor, ms_root, pMsg);
}

void TcpSrvImp::cbOnSrvDisconnected(const AmsgPtr& pMsg)
{
	if (pMsg)
	{
		TcpSDisconnCbAmsg* zp_msg = (TcpSDisconnCbAmsg*)(pMsg.get());
		clearSession(zp_msg->ms_sessionAddr);
		ACTOR_SEND_MSG(mp_srvactor, ms_root, pMsg);
	}
}

void TcpSrvImp::cbOnSrvRecvData(const AmsgPtr& pMsg)
{
	ACTOR_SEND_MSG(mp_srvactor, ms_root, pMsg);
}

void TcpSrvImp::cbOnSrvRecvTestingData(const AmsgPtr& pMsg)
{
	ACTOR_SEND_MSG(mp_srvactor, ms_root, pMsg);
}

///////////////////////////////////////////////////////////////////////////////
// utils

ActorPtr TcpSrvImp::getNewSession(TcpSrvActor* pSrv, uint32 uSrvID, uint32 uSessionID)
{
	if (!pSrv)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::getNewSession, args error", uSrvID);
		return ActorPtr();
	}

	ActorPtr pSession = nullptr;
	try
	{
		std::stringstream addr;
		addr << uSrvID << "_" << uSessionID;
		auto pSelf = pSrv->getActor(pSrv->getAddr());
		pSession = ActorPtr(new TcpSession(pSelf, addr.str(), uSrvID, uSessionID));
	}
	catch (std::exception& e)
	{
		LOGINFO(TI_LIBIPC, "TcpSrvImp::getNewSession, new fail", uSrvID, e.what());
		pSession.reset();
		return pSession;
	}
	return pSession;
}

void TcpSrvImp::clearSession(const std::string& strAddr)
{
	auto pConnIter = mo_mapConnection.find(strAddr);
	if (pConnIter != mo_mapConnection.end())
	{
		mo_mapConnection.erase(pConnIter);
	}

	auto pTimeoutIter = mo_mapTimeoutConnection.find(strAddr);
	if (pTimeoutIter != mo_mapTimeoutConnection.end())
	{
		mo_mapTimeoutConnection.erase(pTimeoutIter);
	}

	auto pTestedIter = mo_mapTested.find(strAddr);
	if (pTestedIter != mo_mapTested.end())
	{
		mo_mapTested.erase(pTestedIter);
	}

	mp_srvactor->removeActor(strAddr);
}

void TcpSrvImp::closeAllSession(const std::string& strMsg, bool bSafe)
{
	for (auto iter = mo_mapConnection.begin(); iter != mo_mapConnection.end(); ++iter)
	{
		ActorPtr zp_sess = std::get<1>(iter->second);
		ACTOR_CAST(TcpSession, zp_sess)->postClose(false, strMsg, bSafe);
	}

	for (auto zp_iter = mo_mapTested.begin(); zp_iter != mo_mapTested.end(); ++zp_iter)
	{
		ActorPtr zp_sess = mp_srvactor->getActor(zp_iter->first);
		if (zp_sess)
		{
			ACTOR_CAST(TcpSession, zp_sess)->postClose(false, strMsg, bSafe);
		}
	}
}

