#ifndef LIBIPC_TCP_CLT_ACTOR_H_
#define LIBIPC_TCP_CLT_ACTOR_H_

#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/actor/base_actor.h"
#include <boost/noncopyable.hpp>
#include <boost/smart_ptr.hpp>

NamespaceStart

class TcpConnection;
class TcpCltActor : public Actor
{
public:
    explicit TcpCltActor(AsioSvc *pSvc, const std::string &strIdent, const std::string &strAddr);

    ~TcpCltActor();

private:
	void process(const std::string& strSender, const AmsgPtr& pMsg);

private:
	boost::shared_ptr<TcpConnection> mp_imp;
};

NamespaceEnd

#endif
