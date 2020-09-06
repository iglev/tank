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
	if (!pMsg)
	{
		return;
	}

	switch (pMsg->mn_msgId)
	{
	case AmsgType::SHUTDOWN_REQ:
	{
		mp_imp->reqShutdown(pMsg);
		break;
	}
	case AmsgType::TCP_S_OPEN:
	{
		// open tcp server
		if (!mp_imp)
		{
			try
			{
				mp_imp = boost::shared_ptr<TcpSrvImp>(new TcpSrvImp(this));
			}
			catch (...)
			{
				mp_imp.reset();
				return;
			}
		}

		TcpSOpenAmsg* zp_msg = (TcpSOpenAmsg*)(pMsg.get());
		mp_imp->onListen(strSender, zp_msg->mn_srvid, zp_msg->ms_ip, zp_msg->mn_port,
			zp_msg->mn_maxLimit, zp_msg->mn_checkSeconds, zp_msg->mn_timeoutSeconds);

		break;
	}
	case AmsgType::TCP_S_SEND:
	{
		// send data
		TcpSSendAmsg* zp_msg = (TcpSSendAmsg*)(pMsg.get());
		SEND_MSG(zp_msg->ms_sessionAddr, pMsg);

		break;
	}
	case AmsgType::TCP_S_CLOSE:
	{
		// close session
		mp_imp->reqClose(pMsg);
		break;
	}
	case AmsgType::TCP_S_CLOSE_SAFE:
	{
		// close session safe
		mp_imp->reqCloseSafe(pMsg);
		break;
	}
	case AmsgType::TCP_S_CLOSE_ALL:
	{
		// close all session
		mp_imp->reqCloseAll(pMsg);
		break;
	}
	case AmsgType::TCP_S_CLOSE_ALL_SAFE:
	{
		// close all session safe
		mp_imp->reqCloseAllSafe(pMsg);
		break;
	}
	case AmsgType::TCP_S_SET_TESTING:
	{
		// set session tested
		mp_imp->reqSetTesting(pMsg);
		break;
	}
	case AmsgType::TCP_S_SET_ACTOR:
	{
		// bind session actor and other actor (for recv cb)
		mp_imp->reqSetActor(pMsg);
		break;
	}
	case AmsgType::TCP_S_CB_CONNECTED:
	{
		// cb session connected
		mp_imp->cbOnSrvConnected(pMsg);
		break;
	}
	case AmsgType::TCP_S_CB_DISCONNECTED:
	{
		// cb session disconnected
		mp_imp->cbOnSrvDisconnected(pMsg);
		break;
	}
	case AmsgType::TCP_S_CB_RECV_DATA:
	{
		// cb session recv data
		mp_imp->cbOnSrvRecvData(pMsg);
		break;
	}
	case AmsgType::TCP_S_CB_RECV_TEST_DATA:
	{
		// cb session recv test data
		mp_imp->cbOnSrvRecvTestingData(pMsg);
		break;
	}
	default:
	{
		break;
	}
	}
}

