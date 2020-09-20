#include "base/typedef.h"
#include "base/exception/extrack.h"
#include "base/actor/base_actor_msg.h"
#include "base/actor/base_actor.h"
#include "base/exector/asio_svr.h"
#include "ipc/ipc_config.h"
#include "ipc/tcp_srv_actor.h"

#include <iostream>
#include <thread>

#ifdef _DEBUG
//#include "vld.h"
#endif

NamespaceUsing

class RootActor : public Actor
{
public:
	explicit RootActor(AsioSvc* pSvc, const std::string& strIdent, const std::string& strAddr)
		: Actor(pSvc, strIdent, strAddr)
	{

	}

	~RootActor()
	{

	}

	void start()
	{
		std::cout << "root start" << std::endl;
		ActorPtr pTcpSrv = ActorPtr(new TcpSrvActor(getSvr(), IPC_TCP_SRV_IDENT, "srv_1"));
		pTcpSrv->addActor(pTcpSrv);

		auto pRoot = getActor(getAddr());
		pRoot->addActor(pTcpSrv);
		pTcpSrv->addActor(pRoot);

		SEND_NEW_MSG(pTcpSrv->getAddr(), TcpSOpenAmsg, (1, "127.0.0.1", 8999, 1024, 3, 5));
	}

	void stop()
	{
		std::cout << "root stop" << std::endl;
		auto pTcpSrv = getActor("srv_1");
		std::cout << pTcpSrv->getAddr() << std::endl;
		SEND_NEW_MSG("srv_1", ShutdownReqAmsg, ());
	}

private:
	void process(const std::string& strSender, const AmsgPtr& pMsg)
	{
		LOGINFO(TI_LIBIPC, pMsg);
		switch (pMsg->mn_msgId)
		{
		case AmsgType::SHUTDOWN_RES:
		{
			std::cout << "shutdown_res " << strSender << std::endl;
			AmsgPtr pKillMsg = AmsgPtr(new KillSelfAmsg(getActor(getAddr())));
			send(getAddr(), pKillMsg);
			break;
		}
		case AmsgType::TCP_S_CB_STARTUP:
		{
			TcpSStartUpCbAmsg* pStartUpMsg = (TcpSStartUpCbAmsg*)(pMsg.get());
			std::cout << "tcp server start up " << pStartUpMsg->mn_srvid << std::endl;
			break;
		}
		}
	}

};

int test1(int argc, char* argv[])
{
	AsioSvc stSvc(8);
	stSvc.start();

	ActorPtr pRoot = ActorPtr(new RootActor(&stSvc, "root", "root"));
	pRoot->addActor(pRoot);

	ACTOR_CAST(RootActor, pRoot)->start();
	while (!stSvc.isPressCtrlC())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	ACTOR_CAST(RootActor, pRoot)->stop();

	std::cout << "stop before" << std::endl;
	while (pRoot->isValid())
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
		std::cout << "1111111" << std::endl;
	}
	std::cout << "stop after" << std::endl;
	stSvc.stop();

	return 0;
}

int main(int argc, char* argv[])
{
	test1(argc, argv);
	return 0;
}
