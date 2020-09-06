#ifndef LIBIPC_TCP_SESSION_H_
#define LIBIPC_TCP_SESSION_H_

#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/actor/base_actor_msg.h"
#include "base/actor/base_actor.h"
#include "ipc/ipc_config.h"
#include "ipc/net_packet.h"

#include <boost/shared_array.hpp>
#include <memory>
#include <string>
#include <queue>
#include <tuple>

NamespaceStart

class TcpSession : public Actor
{
	typedef std::queue<std::tuple<uint32, boost::shared_array<ubyte>>> SendQueue;
public:
	explicit TcpSession(ActorPtr& pSrvActor, const std::string& strAddr, uint32 uSrvID, uint32 uSid);

	~TcpSession();

	boost::asio::ip::tcp::socket& getSocket()
	{
		return mo_socket;
	}

	void startRecv();

	void postClose(bool bCloseByRemote, const std::string& strMsg, bool bSafe);

	void postSetTest(bool bPass);

	void postSetActor(ActorPtr& pActor);

private:
	// test
	void onSetTest(ActorPtr& pSelf, bool bPass);

	void onSetActor(ActorPtr& pSelf, ActorPtr& pActor);

private:
	// recv
	void doStartRecv(ActorPtr& pSelf);

	void doRecv(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize);

	void onRecv(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize);

private:
	// send
	void doSendData(const void* pData, uint32 uLen);

	void onSendData(ActorPtr& pSelf, boost::shared_array<ubyte>& data, uint32 uLen);

	void doSendAfter(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize);

	void onSendAfter(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize);

private:
	// close
	void postCloseIn(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg, bool bSafe);

	void doClose(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg, bool bSafe);

	void onClose(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg);

private:
	// cb
	void cbConnected();

	void cbRecvTestingData(void* pData, uint32 uLen);

	void cbRecvData(void* pData, uint32 uLen);

	void cbDisconnected(bool bCloseByRemote, const std::string& strMsg);

private:
	void process(const std::string& strSender, const AmsgPtr& pMsg);

private:
	uint32 mn_srvid;
	uint32 mn_sid;
	TcpSendState mn_sendState;
	TcpSockState mn_sessionState;
	std::string ms_srvAddr;
	std::string ms_cbAddr;
	boost::asio::ip::tcp::socket mo_socket;

	ubyte* mp_recvBuf;
	uint32 mn_recvBufferSize;

	RecvPacketBuilder* mp_recvBufferBuilder;
	SendQueue* mp_sendQueue;

	ActorPtr mp_self;
};

NamespaceEnd

#endif
