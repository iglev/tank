#ifndef LIBIPC_TCP_SRV_ACTOR_H_
#define LIBIPC_TCP_SRV_ACTOR_H_

#include "base/typedef.h"
#include "base/actor/base_actor.h"
#include "base/exector/asio_svr.h"
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>

NamespaceStart

class TcpSrvImp;
class TcpSrvActor : public Actor
{
public:
	explicit TcpSrvActor(AsioSvc* pSvc, const std::string& strIdent, const std::string& strAddr);

	~TcpSrvActor();

private:
	void process(const std::string& strSender, const AmsgPtr& pMsg);

private:
	boost::shared_ptr<TcpSrvImp> mp_imp;
};

NamespaceEnd

#endif LIBIPC_TCP_SRV_ACTOR_H_
