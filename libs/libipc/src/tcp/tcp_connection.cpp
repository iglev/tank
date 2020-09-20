#include "tcp_connection.h"

NamespaceUsing

TcpConnection::TcpConnection(ActorPtr pCltActor)
    : mb_tested(false)
    , mp_cltactor(pCltActor)
    , mn_sendState(TcpSendState::INIT)
    , mn_sockState(TcpSockState::CLOSED)
    , ms_root("")
    , ms_cbAddr("")
    , mo_socket(mp_cltactor->getSvr()->getSvc())
    , mp_recvBuf(nullptr)
    , mn_recvBufferSize(0)
    , mp_recvBufferBuilder(nullptr)
    , mp_sendQueue(nullptr)
{

}

TcpConnection::~TcpConnection()
{
    safeDeleteArray(mp_recvBuf);
    safeDelete(mp_recvBufferBuilder);
    safeDelete(mp_sendQueue);
}

///////////////////////////////////////////////////////////////////////////////
// connect

void TcpConnection::startConnect(const std::string &strRoot, const std::string &strIp, uint16 uPort)
{
    if(strRoot.empty() || strIp.empty() || uPort <= 0)
    {
        LOGINFO(TI_LIBIPC, "TcpConnection::startConnect, args error", strRoot, strIp, uPort);
        return;
    }

    if(mn_sockState != TcpSockState::CLOSED)
    {
        LOGINFO(TI_LIBIPC, "TcpConnection::startConnect, socket state invalid", strRoot, strIp, uPort, mn_sockState);
        return;
    }
    mn_sockState = TcpSockState::CONNECTING;
    ms_root = strRoot;
    auto endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string(strIp), uPort);
    mo_socket.async_connect(endpoint, 
        boost::bind(&TcpConnection::doConnect, this, mp_cltactor, boost::asio::placeholders::error, endpoint));
}

inline void TcpConnection::doConnect(ActorPtr &pActor, const boost::system::error_code &error, boost::asio::ip::tcp::endpoint &endpoint)
{
    pActor->getChannel().post(boost::bind(&TcpConnection::onConnect, this, pActor, error, endpoint));
}

void TcpConnection::onConnect(ActorPtr &pActor, const boost::system::error_code &error, boost::asio::ip::tcp::endpoint &endpoint)
{
    if(mn_sockState == TcpSockState::CLOSING || mn_sockState == TcpSockState::CLOSED)
    {
        LOGINFO(TI_LIBIPC, "TcpConnection::onConnect, state", mn_sockState);
        return;
    }

    if(error)
    {
        // cb connect fail
        cbConnectFail("TcpConnection::onConnect, " + error.message());
        // close
        onClose(pActor, false, "TcpConnection::onConnect, connect fail");
        return;
    }

    mn_sockState = TcpSockState::CONNECTED;
    try
    {
        // init buffer
        mn_recvBufferSize = PACK_RECVBUFF_LEN;
        mp_recvBuf = new ubyte[mn_recvBufferSize];
        mp_recvBufferBuilder = new RecvPacketBuilder(PACK_RECVBUFF_LEN+PACK_MAX_RECV_LEN);
        mp_sendQueue = new SendQueue();
    }
    catch(std::exception &e)
    {
        safeDeleteArray(mp_recvBuf);
        safeDelete(mp_recvBufferBuilder);
        safeDelete(mp_sendQueue);
        postCloseIn(pActor, false, std::string("TcpConnection::onConnect, ") + e.what(), false);
        return;
    }

    // cb connect
    cbConnect();
    
    // start recv
    startRecv(pActor);
}

///////////////////////////////////////////////////////////////////////////////
// recv

void TcpConnection::startRecv(ActorPtr &pActor)
{
    mo_socket.async_read_some(
        boost::asio::buffer(mp_recvBuf, mn_recvBufferSize),
        boost::bind(&TcpConnection::doRecv, this, pActor, 
            boost::asio::placeholders::error,
            boost::asio::placeholders::bytes_transferred
        )
    );
}

void TcpConnection::doRecv(ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes)
{
    if(error)
    {
        postCloseIn(pActor, true, "TcpConnection::doRecv, "+error.message(), false);
        return;
    }
    pActor->getChannel().post(boost::bind(&TcpConnection::onRecv, this, pActor, error, uBytes));
}

void TcpConnection::onRecv(ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes)
{
    uint32 buffLen = mp_recvBufferBuilder->push(mp_recvBuf, (uint32)(uBytes));
    if(mp_recvBufferBuilder->error_code() != 0)
    {
        postCloseIn(pActor, false, "TcpConnection::onRecv, recv data len had overflow", false);
        return;
    }

    while(buffLen > 0)
    {
        void *pData = mp_recvBufferBuilder->front(buffLen);
        if(mn_sockState == TcpSockState::TESTING)
        {
            // cb recv testing data
            cbRecvTestingData(pData, buffLen);
            mn_sockState = TcpSockState::TESTED;
        }
        else if(mn_sockState == TcpSockState::TESTED)
        {
            // cb recv data
            cbRecvData(pData, buffLen);
        }
        else
        {
            LOGINFO(TI_LIBIPC, "TcpConnection::onRecv, unknown state");
        }
        mp_recvBufferBuilder->pop(buffLen);
        buffLen = mp_recvBufferBuilder->push(nullptr, 0);
    }
    startRecv(pActor);
}

///////////////////////////////////////////////////////////////////////////////
// send

void TcpConnection::doSendData(const void *pData, uint32 uLen)
{
    if(pData == nullptr || uLen <= 0 || uLen > PACK_MAX_SEND_LEN)
    {
        LOGINFO(TI_LIBIPC, "TcpConnection::doSendData, args error", pData == nullptr, uLen);
        return;
    }

    if(!mo_socket.is_open())
    {
        LOGINFO(TI_LIBIPC, "TcpConnection::doSendData, socket not open");
        return;
    }

    if(mn_sockState == TcpSockState::CONNECTED)
    {
        mn_sockState = TcpSockState::TESTING;
    }

    mn_sendState = TcpSendState::SENDING;
    auto data = boost::shared_array<ubyte>(new ubyte[uLen]);
    ::memcpy(data.get(), pData, sizeof(ubyte)*uLen);
    mp_cltactor->getChannel().post(boost::bind(&TcpConnection::onSendData, this, mp_cltactor, data, uLen));
}

void TcpConnection::onSendData(const ActorPtr &pActor, boost::shared_array<ubyte> &pData, uint32 uLen)
{
    bool empty = mp_sendQueue->empty();
    mp_sendQueue->push(std::make_tuple(uLen, pData));
    mn_sendState = TcpSendState::INIT;
    if(empty)
    {
        auto &p = mp_sendQueue->front();
        boost::asio::async_write(mo_socket,
            boost::asio::buffer(std::get<1>(p).get(), std::get<0>(p)),
            boost::bind(&TcpConnection::doSendAfter, this, pActor,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            )
        );
    }
}

inline void TcpConnection::doSendAfter(const ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes)
{
    pActor->getChannel().post(boost::bind(&TcpConnection::onSendAfter, this, pActor, error, uBytes));
}

void TcpConnection::onSendAfter(const ActorPtr &pActor, const boost::system::error_code &error, std::size_t uBytes)
{
    if(error)
    {
        if(mn_sockState != TcpSockState::CLOSED)
        {
            // send fail, socket not close, state = fail
            mn_sockState = TcpSockState::FAILED;
            postCloseIn(pActor, true, "TcpConnection::onSendAfter, "+ error.message(), false);
        }
        return;
    }

    if(mp_sendQueue->empty())
    {
        if(mn_sockState == TcpSockState::CLOSING)
        {
            mp_cltactor->getChannel().post(
                boost::bind(&TcpConnection::onClose, this, pActor,
                    false, "TcpConnection::onSendAfter send queue empty before send, socket closing safety"
                )
            );
        }
        return;
    }

    auto &p = mp_sendQueue->front();
    std::get<1>(p).reset();
    mp_sendQueue->pop();

    if(!mp_sendQueue->empty())
    {
        auto &p = mp_sendQueue->front();
        boost::asio::async_write(mo_socket,
            boost::asio::buffer(std::get<1>(p).get(), std::get<0>(p)),
            boost::bind(&TcpConnection::doSendAfter, this, pActor,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred
            )
        );
    }
    else if(mn_sockState == TcpSockState::CLOSING)
    {
        mp_cltactor->getChannel().post(
            boost::bind(&TcpConnection::onClose, this, pActor,
                false, "TcpConnection::onSendAfter, send queue empty before send, socket closing safety"
            )
        );
    }
}

///////////////////////////////////////////////////////////////////////////////
// close

void TcpConnection::postClose(bool bCloseByRemote, const std::string &strMsg, bool bSafe)
{
    if(mp_cltactor)
    {
        mp_cltactor->getChannel().post(boost::bind(&TcpConnection::doClose, this, mp_cltactor, bCloseByRemote, strMsg, bSafe));
    }
}

void TcpConnection::postCloseIn(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg, bool bSafe)
{
    pActor->getChannel().post(boost::bind(&TcpConnection::doClose, this, pActor, bClosedByRemote, strMsg, bSafe));
}

void TcpConnection::doClose(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg, bool bSafe)
{
    if(mn_sockState == TcpSockState::CLOSED || mn_sockState == TcpSockState::CLOSING)
    {
        return;
    }

    mn_sockState = TcpSockState::CLOSING;
    if(bSafe && (mp_sendQueue && (!mp_sendQueue->empty() || mn_sendState == TcpSendState::SENDING)))
    {
        // wait send all data
        return;
    }
    onClose(pActor, bClosedByRemote, strMsg);
}

void TcpConnection::onClose(const ActorPtr &pActor, bool bClosedByRemote, const std::string &strMsg)
{
    if(mn_sockState == TcpSockState::CLOSED)
    {
        return;
    }
    mn_sockState = TcpSockState::CLOSED;

    if(mo_socket.is_open())
    {
        boost::system::error_code error;
        mo_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
        mo_socket.close(error);
    }

    // clear send queue
    if(mp_sendQueue)
    {
        while(!mp_sendQueue->empty())
        {
            auto &p = mp_sendQueue->front();
            std::get<1>(p).reset();
            mp_sendQueue->pop();
        }
    }

    // cb disconnected
    cbDisconnect(bClosedByRemote, strMsg);

    // kill self
    ACTOR_SEND_NEW_MSG(mp_cltactor, mp_cltactor->getAddr(), KillSelfAmsg, (mp_cltactor));
    mp_cltactor = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// cb

void TcpConnection::cbConnect()
{
    ACTOR_SEND_NEW_MSG(mp_cltactor, ms_root, TcpCConnCbAmsg, (mp_cltactor->getAddr()));
}

void TcpConnection::cbConnectFail(const std::string &strMsg)
{
    ACTOR_SEND_NEW_MSG(mp_cltactor, ms_root, TcpCConnFailCbAmsg, (mp_cltactor->getAddr(), strMsg));
}

void TcpConnection::cbDisconnect(bool bClosedByRemote, const std::string &strMsg)
{
    ACTOR_SEND_NEW_MSG(mp_cltactor, ms_root, TcpCDissconnCbAmsg, (mp_cltactor->getAddr(), strMsg, bClosedByRemote));
}

void TcpConnection::cbRecvTestingData(void *pData, uint32 uLen)
{
    bool bFlag = true;
	NetPacket2 packet(pData, uLen);
	uint32 msgId = 0;
	if (packet.pop_uint32(&msgId) <= 0)
	{
		bFlag = false;
	}

	uint32 uSid = 0;
	if (packet.pop_uint32(&uSid) <= 0)
	{
		bFlag = false;
	}

	uint8 uMsgType = 0;
	if (packet.pop_uint8(&uMsgType) <= 0)
	{
		bFlag = false;
	}

	if (bFlag)
	{
		std::string data;
		packet.pop_string(&data);
		if (ms_cbAddr.empty())
		{
			ACTOR_SEND_NEW_MSG(mp_cltactor, ms_root, TcpCRecvTestCbAmsg, (
				mp_cltactor->getAddr(), msgId, uSid, uMsgType, data
			));
		}
		else
		{
			ACTOR_SEND_NEW_MSG(mp_cltactor, ms_cbAddr, TcpCRecvTestCbAmsg, (
				mp_cltactor->getAddr(), msgId, uSid, uMsgType, data
			));
		}
	}

}

void TcpConnection::cbRecvData(void *pData, uint32 uLen)
{
	bool bFlag = true;
	NetPacket2 packet(pData, uLen);
	uint32 uMsgId = 0;
	if (packet.pop_uint32(&uMsgId) <= 0)
	{
		bFlag = false;
	}

	uint32 uSid = 0;
	if (packet.pop_uint32(&uSid) <= 0)
	{
		bFlag = false;
	}

	uint8 uMsgType = 0;
	if (packet.pop_uint8(&uMsgType) <= 0)
	{
		bFlag = false;
	}

	if (bFlag)
	{
		std::string data;
		packet.pop_string(&data);
		if (ms_cbAddr.empty())
		{
			ACTOR_SEND_NEW_MSG(mp_cltactor, ms_root, TcpCRecvCbAmsg, (
				mp_cltactor->getAddr(), uMsgId, uSid, uMsgType, data
			));
		}
		else
		{
			ACTOR_SEND_NEW_MSG(mp_cltactor, ms_cbAddr, TcpCRecvCbAmsg, (
				mp_cltactor->getAddr(), uMsgId, uSid, uMsgType, data
			));
		}
	}
}

