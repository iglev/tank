#include "ipc/tcp_clt_actor.h"
#include "tcp_connection.h"
#include <vector>

NamespaceUsing

TcpCltActor::TcpCltActor(AsioSvc *pSvc, const std::string &strIdent, const std::string &strAddr)
    : Actor(pSvc, strIdent, strAddr)
    , mp_imp(nullptr)
{

}

TcpCltActor::~TcpCltActor()
{
    mp_imp.reset();
}

void TcpCltActor::process(const std::string& strSender, const AmsgPtr& pMsg)
{
    if(!pMsg)
    {
        return;
    }

	switch (pMsg->mn_msgId)
	{
	case AmsgType::SHUTDOWN_REQ:
	{
		if (!mp_imp)
		{
			SEND_NEW_MSG(strSender, ShutdownResAmsg, ());
		}
		else
		{
			mp_imp->postClose(false, "TcpCltActor::process, postClose connection", true);
		}

		break;
	}
	case AmsgType::TCP_C_CONNECT:
	{
		if (!mp_imp)
		{
			try
			{
				mp_imp = boost::shared_ptr<TcpConnection>(new TcpConnection(getActor(getAddr())));
			}
			catch (...)
			{
				LOGINFO(TI_LIBIPC, "TcpCltActor::process, connect new fail");
				mp_imp.reset();
				return;
			}
		}

		TcpCConnectAmsg *pConnectAmsg = (TcpCConnectAmsg*)(pMsg.get());
		mp_imp->startConnect(strSender, pConnectAmsg->ms_ip, pConnectAmsg->mn_port);

		break;
	}
	case AmsgType::TCP_C_SEND:
	{
		if (!mp_imp)
		{
			return;
		}

		TcpCSendAmsg *pSendAmsg = (TcpCSendAmsg*)(pMsg.get());
		NetPacket2 zo_packet((uint16)(11 + pSendAmsg->ms_msg.size()));
		if (!pSendAmsg->ms_msg.empty())
		{
			zo_packet.push_string(pSendAmsg->ms_msg);
		}
		zo_packet.push_uint8(pSendAmsg->mn_msgtype);
		zo_packet.push_uint32(pSendAmsg->mn_sid);
		zo_packet.push_uint32(pSendAmsg->mn_id);
		mp_imp->doSendData(zo_packet.buff(), zo_packet.buff_len());

		break;
	}
	case AmsgType::TCP_C_CLOSE:
	{
		if (!mp_imp)
		{
			return;
		}
		mp_imp->postClose(false, "TcpCltActor::process, close connection", false);

		break;
	}
	case AmsgType::TCP_C_CLOSE_SAFE:
	{
		if (!mp_imp)
		{
			return;
		}
		mp_imp->postClose(false, "TcpCltActor::process, close_safe connection", true);

		break;
	}
	case AmsgType::TCP_C_IPS_REQ:
	{
		if (!mp_imp)
		{
			return;
		}

		TcpCIpsReqAmsg *pIpsReqAmsg = (TcpCIpsReqAmsg*)(pMsg.get());
		std::vector<std::string> vecIps;
		try
		{
			boost::asio::ip::tcp::resolver resolver(getSvr()->getSvc());
			boost::asio::ip::tcp::resolver::query query(pIpsReqAmsg->ms_url, "");
			auto iter = resolver.resolve(query);
			boost::asio::ip::tcp::resolver::iterator iterEnd;
			for (; iter != iterEnd; ++iter)
			{
				vecIps.push_back((*iter).endpoint().address().to_string());
			}
		}
		catch (boost::system::system_error &e)
		{
			LOGINFO(TI_LIBIPC, "TcpCltActor::process, ips req", e.what(), pIpsReqAmsg->ms_url);
		}
		SEND_NEW_MSG(mp_imp->getRoot(), TcpCIpsResAmsg, (vecIps));
		break;
	}
	default:
	{
		break;
	}
	}
}

