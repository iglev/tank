#include "ipc/tcp_srv_actor.h"
#include "tcp/tcp_srv_imp.h"

NamespaceUsing

TcpSrvActor::TcpSrvActor(AsioSvc* pSvc, const std::string& strIdent, const std::string& strAddr)
	: Actor(pSvc, strIdent, strAddr)
	, mp_imp(nullptr)
{

}

TcpSrvActor::~TcpSrvActor()
{
	mp_imp.reset();
}

void TcpSrvActor::process(const std::string& strSender, const AmsgPtr& pMsg)
{

}

