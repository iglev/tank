#include "tcp/tcp_session.h"
#include <iostream>

NamespaceUsing

///////////////////////////////////////////////////////////////////////////////
// public

TcpSession::TcpSession(ActorPtr& pSrvActor, const std::string& strAddr, uint32 uSrvID, uint32 uSid)
	: Actor(pSrvActor->getSvr(), IPC_TCP_SESSION_IDENT, strAddr)
	, mn_srvid(uSrvID)
	, mn_sid(uSid)
	, mn_sendState(TcpSendState::INIT)
	, mn_sessionState(TcpSockState::CLOSED)
	, ms_srvAddr(pSrvActor->getAddr())
	, ms_cbAddr("")
	, mo_socket(pSrvActor->getSvr()->getSvc())
	, mp_recvBuf(nullptr)
	, mn_recvBufferSize(0)
	, mp_recvBufferBuilder(nullptr)
	, mp_sendQueue(nullptr)
	, mp_self(nullptr)
{
	this->addActor(pSrvActor);
	this->setRoute(ms_srvAddr);
}

TcpSession::~TcpSession()
{
	safeDeleteArray(mp_recvBuf);
	safeDelete(mp_recvBufferBuilder);
	safeDelete(mp_sendQueue);
}

///////////////////////////////////////////////////////////////////////////////
// test

void TcpSession::postSetTest(bool bPass)
{
	getChannel().post(boost::bind(&TcpSession::onSetTest, this, mp_self, bPass));
}

void TcpSession::onSetTest(ActorPtr& pSelf, bool bPass)
{
	if (mn_sessionState == TcpSockState::TESTING)
	{
		if (bPass)
		{
			mn_sessionState = TcpSockState::TESTED;
		}
		else
		{
			postClose(false, "TcpSession::onSetTest", false);
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// set actor

void TcpSession::postSetActor(ActorPtr& pActor)
{
	getChannel().post(boost::bind(&TcpSession::onSetActor, this, mp_self, pActor));
}

void TcpSession::onSetActor(ActorPtr& pSelf, ActorPtr& pActor)
{
	if (!pActor)
	{
		LOGINFO(TI_LIBIPC, "TcpSession::onSetActor, args error", mn_srvid, mn_sid);
		return;
	}
	ms_cbAddr = pActor->getAddr();
	addActor(pActor);
}

///////////////////////////////////////////////////////////////////////////////
// recv

void TcpSession::startRecv()
{
	if (mn_sessionState != TcpSockState::CLOSED)
	{
		LOGINFO(TI_LIBIPC, "TcpSession::startRecv, state", mn_sessionState);
		return;
	}
	
	mn_sessionState = TcpSockState::TESTING;
	try
	{
		// init buffer
		mn_recvBufferSize = PACK_RECVBUFF_LEN;
		mp_recvBuf = new ubyte[mn_recvBufferSize];
		mp_recvBufferBuilder = new RecvPacketBuilder(PACK_RECVBUFF_LEN + PACK_MAX_RECV_LEN);
		mp_sendQueue = new SendQueue;
	}
	catch (...)
	{
		safeDeleteArray(mp_recvBuf);
		safeDelete(mp_recvBufferBuilder);
		safeDelete(mp_sendQueue);
		postClose(false, "TcpSession::startRecv, new buffer fail", false);
	}

	// cb connected
	cbConnected();
	mp_self = getActor(getAddr());
	getChannel().post(boost::bind(&TcpSession::doStartRecv, this, mp_self));
}

inline void TcpSession::doStartRecv(ActorPtr& pSelf)
{
	mo_socket.async_read_some(
		boost::asio::buffer(mp_recvBuf, mn_recvBufferSize),
		boost::bind(&TcpSession::doRecv, this, mp_self,
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void TcpSession::doRecv(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize)
{
	if (error)
	{
		postCloseIn(pSelf, true, "TcpSession::doRecv, " + error.message(), false);
	}
	getChannel().post(boost::bind(&TcpSession::onRecv, this, pSelf, error, uSize));
}

void TcpSession::onRecv(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize)
{
	uint32 uBufferLen = mp_recvBufferBuilder->push(mp_recvBuf, (uint32)uSize);
	if (mp_recvBufferBuilder->error_code() != 0)
	{
		postClose(false, "TcpSession::onRecv, recv data len had overflow", false);
	}

	while (uBufferLen > 0)
	{
		void* pBuffer = mp_recvBufferBuilder->front(uBufferLen);
		if (mn_sessionState == TcpSockState::TESTING)
		{
			// cb recv testing data
			cbRecvTestingData(pBuffer, uBufferLen);
		}
		else if (mn_sessionState == TcpSockState::TESTED)
		{
			// cb recv data
			cbRecvData(pBuffer, uBufferLen);
		}
		else
		{
			LOGINFO(TI_LIBIPC, "TcpSession::onRecv, unknown state", mn_srvid, mn_sid);
		}
		mp_recvBufferBuilder->pop(uBufferLen);
		uBufferLen = mp_recvBufferBuilder->push(nullptr, 0);
	}
	doStartRecv(pSelf);
}

///////////////////////////////////////////////////////////////////////////////
// send

void TcpSession::doSendData(const void* pData, uint32 uLen)
{
	if (pData == nullptr || uLen <= 0 || uLen > PACK_MAX_SEND_LEN)
	{
		LOGINFO(TI_LIBIPC, "TcpSession::doSendData, args error", pData == nullptr, uLen);
		return;
	}

	if ((!mo_socket.is_open()) || (mn_sessionState != TcpSockState::TESTED))
	{
		LOGINFO(TI_LIBIPC, "TcpSession::doSendData, state error",
			mn_srvid, mn_sid, mo_socket.is_open(), mn_sessionState);
		return;
	}
	mn_sendState = TcpSendState::SENDING;
	auto pTmpData = boost::shared_array<ubyte>(new ubyte[uLen]);
	::memcpy(pTmpData.get(), pData, sizeof(ubyte) * uLen);
	getChannel().post(boost::bind(&TcpSession::onSendData, this, mp_self, pTmpData, uLen));
}

void TcpSession::onSendData(ActorPtr& pSelf, boost::shared_array<ubyte>& data, uint32 uLen)
{
	bool bEmpty = mp_sendQueue->empty();
	mp_sendQueue->push(std::make_tuple(uLen, data));
	mn_sendState = TcpSendState::INIT;
	if (bEmpty)
	{
		auto& p = mp_sendQueue->front();
		boost::asio::async_write(mo_socket,
			boost::asio::buffer(std::get<1>(p).get(), std::get<0>(p)),
			boost::bind(&TcpSession::doSendAfter, this, pSelf,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
	}
}

void TcpSession::doSendAfter(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize)
{
	getChannel().post(boost::bind(&TcpSession::onSendAfter, this, pSelf, error, uSize));
}

void TcpSession::onSendAfter(ActorPtr& pSelf, const boost::system::error_code& error, std::size_t uSize)
{
	if (error)
	{
		if (mn_sessionState != TcpSockState::CLOSED)
		{
			// send fail, socket not close, state = fail
			mn_sessionState = TcpSockState::FAILED;
			postCloseIn(pSelf, true, "TcpSession::onSendAfter, " + error.message(), false);
		}
		return;
	}

	if (mp_sendQueue->empty())
	{
		if (mn_sessionState == TcpSockState::CLOSING)
		{
			getChannel().post(
				boost::bind(&TcpSession::onClose, this, pSelf, false,
					"TcpSession::onSendAfter send queue empty before send, socket closing safety")
			);
		}
		return;
	}

	auto& p = mp_sendQueue->front();
	std::get<1>(p).reset();
	mp_sendQueue->pop();

	if (!mp_sendQueue->empty())
	{
		auto& p = mp_sendQueue->front();
		boost::asio::async_write(mo_socket,
			boost::asio::buffer(std::get<1>(p).get(), std::get<0>(p)),
			boost::bind(&TcpSession::doSendAfter, this, pSelf,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
	}
	else if (mn_sessionState == TcpSockState::CLOSING)
	{
		getChannel().post(
			boost::bind(&TcpSession::onClose, this, pSelf, false,
				"TcpSession::onSendAfter send queue empty before send, socket closing safety")
		);
	}
}

///////////////////////////////////////////////////////////////////////////////
// close

void TcpSession::postClose(bool bCloseByRemote, const std::string& strMsg, bool bSafe)
{
	getChannel().post(boost::bind(&TcpSession::doClose, this, mp_self, bCloseByRemote, strMsg, bSafe));
}

void TcpSession::postCloseIn(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg, bool bSafe)
{
	getChannel().post(boost::bind(&TcpSession::doClose, this, pSelf, bCloseByRemote, strMsg, bSafe));
}

void TcpSession::doClose(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg, bool bSafe)
{
	if (mn_sessionState == TcpSockState::CLOSED || mn_sessionState == TcpSockState::CLOSING)
	{
		return;
	}

	mn_sessionState = TcpSockState::CLOSING;
	if (bSafe && (mp_sendQueue && (!mp_sendQueue->empty() || mn_sendState == TcpSendState::SENDING)))
	{
		// wait send all data
		return;
	}
	onClose(pSelf, bCloseByRemote, strMsg);
}

void TcpSession::onClose(ActorPtr& pSelf, bool bCloseByRemote, const std::string& strMsg)
{
	if (mn_sessionState == TcpSockState::CLOSED)
	{
		return;
	}

	mn_sessionState = TcpSockState::CLOSED;
	if (mo_socket.is_open())
	{
		boost::system::error_code error;
		mo_socket.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
		mo_socket.close(error);
	}

	// clear send queue
	if (mp_sendQueue)
	{
		while (!mp_sendQueue->empty())
		{
			auto& p = mp_sendQueue->front();
			std::get<1>(p).reset();
			mp_sendQueue->pop();
		}
	}

	// cb send to acceptor, session close
	cbDisconnected(bCloseByRemote, strMsg);

	// kill self
	SEND_NEW_MSG(getAddr(), KillSelfAmsg, (mp_self));
	mp_self = nullptr;
}

///////////////////////////////////////////////////////////////////////////////
// cb

void TcpSession::cbConnected()
{
	boost::system::error_code error;
	std::string strAddr;
	auto addr = mo_socket.remote_endpoint(error);
	if (error)
	{
		strAddr = "";
		LOGINFO("TcpSession::cbConnected, remote_endpoint error:", error.message());
	}
	else
	{
		strAddr = addr.address().to_string();
	}
	SEND_NEW_MSG(ms_srvAddr, TcpSConnCbAmsg, (
		mn_srvid,
		getAddr(),
		strAddr
	));
}

void TcpSession::cbRecvTestingData(void* pData, uint32 uLen)
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
			SEND_NEW_MSG(ms_srvAddr, TcpSRecvTestCbAmsg, (
				mn_srvid, getAddr(), uMsgId, uSid, uMsgType, data
			));
		}
		else
		{
			SEND_NEW_MSG(ms_cbAddr, TcpSRecvTestCbAmsg, (
				mn_srvid, getAddr(), uMsgId, uSid, uMsgType, data
			));
		}
	}
}

void TcpSession::cbRecvData(void* pData, uint32 uLen)
{
	bool bFlag = true;
	NetPacket2 packet(pData, uLen);
	uint32 uMsgID = 0;
	if (packet.pop_uint32(&uMsgID) <= 0)
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
			SEND_NEW_MSG(ms_srvAddr, TcpSRecvCbAmsg, (
				mn_srvid, getAddr(), uMsgID, uSid, uMsgType, data
				));
		}
		else
		{
			SEND_NEW_MSG(ms_cbAddr, TcpSRecvCbAmsg, (
				mn_srvid, getAddr(), uMsgID, uSid, uMsgType, data
				));
		}
	}
}

void TcpSession::cbDisconnected(bool bCloseByRemote, const std::string& strMsg)
{
	SEND_NEW_MSG(ms_srvAddr, TcpSDisconnCbAmsg, (mn_srvid, getAddr(), bCloseByRemote, strMsg));
}

///////////////////////////////////////////////////////////////////////////////

void TcpSession::process(const std::string& strSender, const AmsgPtr& pMsg)
{
	if (!pMsg)
	{
		return;
	}

	switch (pMsg->mn_msgId)
	{
	case AmsgType::TCP_S_SEND:
	{
		TcpSSendAmsg* pSendMsg = (TcpSSendAmsg*)(pMsg.get());
		NetPacket2 packet((uint16)(11 + pSendMsg->ms_msg.size()));
		if (!pSendMsg->ms_msg.empty())
		{
			packet.push_string(pSendMsg->ms_msg);
		}
		packet.push_uint8(pSendMsg->mn_msgtype);
		packet.push_uint32(pSendMsg->mn_sid);
		packet.push_uint32(pSendMsg->mn_id);
		doSendData(packet.buff(), packet.buff_len());
;		break;
	}
	default:
	{
		break;
	}
	}
}
