#ifndef LIBIPC_TCP_SRV_IMP_H_
#define LIBIPC_TCP_SRV_IMP_H_

#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/actor/base_actor_msg.h"
#include "base/actor/base_actor.h"
#include "base/utils/nuid.h"
#include "ipc/ipc_config.h"
#include "ipc/tcp_srv_actor.h"
#include "tcp/tcp_session.h"

#include <boost/noncopyable.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio.hpp>

#include <atomic>
#include <tuple>
#include <unordered_map>

NamespaceStart

class TcpSrvImp : boost::noncopyable
{
public:
	explicit TcpSrvImp(TcpSrvActor* pSrvActor);

	~TcpSrvImp();

public:
	// listen
	bool onListen(const std::string& strRoot, uint32 uSrvID, const std::string& strIP, 
		uint16 uPort, uint32 uMaxLimit, uint32 uCheckTime = 1, uint32 uTimeout = 5);

public:
	// req
	void reqShutdown(const AmsgPtr& pMsg);

	void reqClose(const AmsgPtr& pMsg);

	void reqCloseSafe(const AmsgPtr& pMsg);

	void reqCloseAll(const AmsgPtr& pMsg);

	void reqCloseAllSafe(const AmsgPtr& pMsg);

	void reqSetTesting(const AmsgPtr& pMsg);

	void reqSetActor(const AmsgPtr& pMsg);

private:
	// accepted fail
	void doAcceptedFail(const boost::system::error_code& error, ActorPtr& pSession);

	// accepted success
	void doAccepted(const boost::system::error_code& error, ActorPtr& pSession);

	// accepted
	void onAccepted(ActorPtr& pSession);

private:
	// accept
	void onAccept();

private:
	void doExpired(const boost::system::error_code& err);

	void onExpired();

public:
	// cb
	void cbOnSrvStartup();

	void cbOnSrvShutdown();

	void cbOnSrvConnected(const AmsgPtr &pMsg);

	void cbOnSrvDisconnected(const AmsgPtr& pMsg);

	void cbOnSrvRecvData(const AmsgPtr& pMsg);

	void cbOnSrvRecvTestingData(const AmsgPtr& pMsg);

private:
	// utils
	ActorPtr getNewSession(TcpSrvActor* pSrv, uint32 uSrvID, uint32 uSessionID);

	void clearSession(const std::string& strAddr);

	void closeAllSession(const std::string& strMsg, bool bSafe);

private:
	TcpSrvActor* mp_srvactor;
	boost::asio::ip::tcp::acceptor* mp_acceptor;
	boost::asio::steady_timer* mp_timer;

	std::atomic<bool> mb_accepting;
	std::atomic<bool> mb_closing;
	uint32 mn_srvid;
	uint32 mn_maxLimit;
	uint32 mn_checkTime;
	uint32 mn_timeout;
	boost::asio::ip::tcp::endpoint mo_endpoint;

	Nuid32Year128 mo_id;
	std::string ms_root;

	std::unordered_map<std::string, std::tuple<std::time_t, ActorPtr> > mo_mapConnection;
	std::unordered_map<std::string, ActorPtr> mo_mapTimeoutConnection;
	std::unordered_map<std::string, std::time_t> mo_mapTested;
};

NamespaceEnd

#endif

