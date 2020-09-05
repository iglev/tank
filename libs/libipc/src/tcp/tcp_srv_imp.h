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

