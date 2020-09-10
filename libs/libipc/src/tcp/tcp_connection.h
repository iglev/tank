#ifndef LIBIPC_TCP_CONNECTION_H_
#define LIBIPC_TCP_CONNECTION_H_

#include "base/typedef.h"
#include "base/actor/base_actor_msg.h"
#include "base/actor/base_actor.h"
#include "ipc/ipc_config.h"
#include "ipc/tcp_clt_actor.h"
#include "ipc/net_packet.h"
#include <boost/noncopyable.hpp>
#include <boost/shared_array.hpp>
#include <memory>
#include <string>
#include <queue>
#include <tuple>

NamespaceStart

class TcpConnection : boost::noncopyable
{
    typedef std::queue<std::tuple<uint32, boost::shared_array<ubyte>>> SendQueue;
public:
    explicit TcpConnection(ActorPtr pCltActor);

    ~TcpConnection();

    std::string getRoot() const
    {
        return ms_root;
    }

public:
    void startConnect(const std::string &strRoot, const std::string &strIp, uint16 uPort);

    void doSendData(const void *pData, uint32 uLen);

    void postClose(bool bCloseByRemote, const std::string &strMsg, bool bSafe);

private:
    // connect
    void doConnect(ActorPtr &pActor, const boost::system::error_code &error, boost::asio::ip::tcp::endpoint &endpoint);

    void onConnect(ActorPtr &pActor, const boost::system::error_code &error, boost::asio::ip::tcp::endpoint &endpoint);

private:
    // recv
    void startRecv(ActorPtr &pActor);

    void doRecv(ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes);

    void onRecv(ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes);

private:
	// send
	void onSendData(const ActorPtr &pActor, boost::shared_array<ubyte> &pData, uint32 uLen);

	void doSendAfter(const ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes);

	void onSendAfter(const ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes);

private:
	// close
	void postCloseIn(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg, bool bSafe);

	void doClose(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg, bool bSafe);

	void onClose(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg);

private:
	// cb
	void cbConnect();

	void cbConnectFail(const std::string &strMsg);

	void cbDisconnect(bool bClosedByRemote, const std::string &strMsg);

	void cbRecvTestingData(void *pData, uint32 uLen);

	void cbRecvData(void *pData, uint32 uLen);

private:
    bool mb_tested;
    ActorPtr mp_cltactor;
    TcpSendState mn_sendState;
    TcpSockState mn_sockState;
    std::string ms_root;
    std::string ms_cbAddr;
    boost::asio::ip::tcp::socket mo_socket;

	ubyte* mp_recvBuf;
	uint32 mn_recvBufferSize;

	RecvPacketBuilder* mp_recvBufferBuilder;
	SendQueue* mp_sendQueue;
};

NamespaceEnd

#endif
