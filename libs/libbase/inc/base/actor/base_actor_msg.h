#ifndef LIBBASE_ACTOR_MSG_H_
#define LIBBASE_ACTOR_MSG_H_

#include "base/typedef.h"
#include <msgpack.hpp>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <memory>

// utils macro
#define SEND_MSG(RECV, AMSG) do{ \
		(send(RECV, AMSG)); \
	}while(0)

#define ACTOR_SEND_MSG(ACTOR, RECV, AMSG) do{ \
		if(ACTOR) {\
			((ACTOR)->send(RECV, AMSG)); \
		}\
	}while(0)

#define SEND_NEW_MSG(RECV, MSGTYPE, PARAM) do{ \
		(send(RECV,(AmsgPtr(new MSGTYPE PARAM)))); \
	}while(0)

#define ACTOR_SEND_NEW_MSG(ACTOR, RECV, MSGTYPE, PARAM) do{ \
		if(ACTOR) {\
			((ACTOR)->send(RECV,(AmsgPtr(new MSGTYPE PARAM)))); \
		}\
	}while(0)

NamespaceStart

enum AmsgType
{
	// Actor �ڲ�Э��id
	ACTOR_INNER_MSG_ROUTE     = 1, // route
	ACTOR_INNER_MSG_KILL_SELF = 2, // kill self
	ACTOR_INNER_MSG_ADD_ACTOR = 3, // add actor
	ACTOR_INNER_MSG_DEL_ACTOR = 4, // del actor

	ACTOR_INNER_MSG_GUARD = 1000,

	// Actor �ⲿЭ��id

	ECHO_MSG = 1001, // echo msg

	// common
	SHUTDOWN_REQ = 1100, // shutdown req
	SHUTDOWN_RES = 1101, // shutdown res

	// tcp
	TCP_S_OPEN              = 1610,		// tcp s create srv
	TCP_S_SEND              = 1611,		// tcp s send
	TCP_S_CLOSE             = 1612,		// tcp s close session
	TCP_S_CLOSE_SAFE        = 1613,		// tcp s close session by safety
	TCP_S_CLOSE_ALL         = 1614,		// tcp s close all session
	TCP_S_CLOSE_ALL_SAFE    = 1615,		// tcp s close all session by safety
	TCP_S_SET_TESTING       = 1616,		// tcp s session set testing
	TCP_S_SET_ACTOR         = 1617,		// tcp s session set actor
	TCP_S_CB_STARTUP        = 1618,		// tcp s startup cb
	TCP_S_CB_SHUTDOWN       = 1619,		// tcp s shutdown cb
	TCP_S_CB_CONNECTED      = 1620,		// tcp s connected cb
	TCP_S_CB_DISCONNECTED   = 1621,		// tcp s disconnected cb
	TCP_S_CB_RECV_DATA      = 1622,		// tcp s recv data cb
	TCP_S_CB_RECV_TEST_DATA = 1623,		// tcp s recv testing data cb
	TCP_S_CB_ERROR          = 1624,     // tcp s error cb

	TCP_C_CONNECT           = 1710,     // tcp c connect
	TCP_C_SEND              = 1711,     // tcp c send
	TCP_C_CLOSE             = 1712,     // tcp c close
	TCP_C_CLOSE_SAFE        = 1713,     // tcp c close safe
	TCP_C_IPS_REQ           = 1714,     // tcp c ips req
	TCP_C_IPS_RES           = 1715,     // tcp c ips res
	TCP_C_CB_CONNECTED      = 1716,     // tcp c connected cb
	TCP_C_CB_CONNECT_FAIL   = 1717,     // tcp c connect fail cb
	TCP_C_CB_DISCONNECTED   = 1718,     // tcp c disconnected
	TCP_C_CB_RECV_DATA      = 1719,     // tcp c recv data cb
	TCP_C_CB_RECV_TEST_DATA = 1720,     // tcp c recv test data cb
	TCP_C_CB_ERROR          = 1721,     // tcp c error cb
};

class Amsg
{
public:
	Amsg(int iMsgId): mn_msgId(iMsgId) {}

	virtual void Marshal(std::string& strData) = 0;
	virtual void Unmarshal(const std::string& strData) = 0;
	virtual bool IsCanMarshal() = 0;

	template<typename ...T>
	void MarshalIn(std::string &strData, T&& ...args)
	{
		std::stringstream buffer;
		msgpack::pack(buffer, msgpack::type::make_tuple(std::forward<T>(args)...));
		strData = std::move(buffer.str());
	}

	template<typename ...T>
	void UnmarshalIn(const std::string& strData, T&& ...args)
	{
		msgpack::object_handle obj = msgpack::unpack(strData.data(), strData.size());
		auto tup = msgpack::type::make_tuple(std::forward<T>(args)...);
		obj.get().convert(tup);
		std::tie(std::forward<T>(args)...) = std::move(tup);
	}

public:
	int32 mn_msgId;
};

class Actor;
typedef std::shared_ptr<Actor> ActorPtr;
typedef std::shared_ptr<Amsg> AmsgPtr;

#define CAN_MARSHAL(...) void Marshal(std::string& strData) { \
	MarshalIn(strData, mn_msgId, __VA_ARGS__); } \
	void Unmarshal(const std::string& strData) { \
	UnmarshalIn(strData, mn_msgId, __VA_ARGS__); } \
	inline bool IsCanMarshal() { return true; }

#define CANNOT_MARSHAL(...) void Marshal(std::string& strData) {} \
	void Unmarshal(const std::string& strData) {} \
	inline bool IsCanMarshal() { return false; }


//////////////////////////////////////////////////////////////////////
// echo msg

class EchoMsgAmsg : public Amsg
{
public:
	explicit EchoMsgAmsg(const std::string& strMsg)
		: Amsg(AmsgType::ECHO_MSG)
		, ms_msg(strMsg) {}
	CAN_MARSHAL(ms_msg)
public:
	std::string ms_msg;
};

//////////////////////////////////////////////////////////////////////
// inner

class RouteAmsg : public Amsg
{
public:
	explicit RouteAmsg(const std::string &strSender, const std::string strReceiver, const AmsgPtr&pMsg)
		: Amsg(AmsgType::ACTOR_INNER_MSG_ROUTE)
		, ms_sender(strSender)
		, ms_receiver(strReceiver)
		, mp_msg(pMsg) {}
	CANNOT_MARSHAL()
public:
	std::string ms_sender;
	std::string ms_receiver;
	AmsgPtr mp_msg;
};

class KillSelfAmsg : public Amsg
{
public:
	explicit KillSelfAmsg(const ActorPtr &pSelf = nullptr)
		: Amsg(AmsgType::ACTOR_INNER_MSG_KILL_SELF)
		, mp_self(pSelf) {}
	CANNOT_MARSHAL()
public:
	ActorPtr mp_self;
};

class AddActorAmsg : public Amsg
{
public:
	explicit AddActorAmsg(const std::vector<ActorPtr>& vecList)
		: Amsg(AmsgType::ACTOR_INNER_MSG_ADD_ACTOR)
		, mo_list(vecList) {}
	CANNOT_MARSHAL()
public:
	std::vector<ActorPtr> mo_list;
};

class DelActorAmsg : public Amsg
{
public:
	explicit DelActorAmsg(const std::vector<std::string>& vecList)
		: Amsg(AmsgType::ACTOR_INNER_MSG_DEL_ACTOR)
		, mo_list(vecList) {}
	CANNOT_MARSHAL()
public:
	std::vector<std::string> mo_list;
};

////////////////////////////////////////////////////////////////////////
// common

class ShutdownReqAmsg : public Amsg
{
public:
	explicit ShutdownReqAmsg()
		: Amsg(AmsgType::SHUTDOWN_REQ) {}
	CANNOT_MARSHAL()
};

class ShutdownResAmsg : public Amsg
{
public:
	explicit ShutdownResAmsg()
		: Amsg(AmsgType::SHUTDOWN_RES) {}
	CANNOT_MARSHAL();
};

////////////////////////////////////////////////////////////////////////
// tcp srv

class TcpSOpenAmsg : public Amsg
{
public:
	explicit TcpSOpenAmsg(uint32 uSrvID, const std::string &strIP, uint16 uPort, 
		uint32 uMaxLimit, uint32 uCheckSeconds, uint32 uTimeoutSeconds)
		: Amsg(AmsgType::TCP_S_OPEN)
		, mn_srvid(uSrvID)
		, mn_maxLimit(uMaxLimit)
		, mn_checkSeconds(uCheckSeconds)
		, mn_timeoutSeconds(uTimeoutSeconds)
		, mn_port(uPort)
		, ms_ip(strIP) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	uint32 mn_maxLimit;
	uint32 mn_checkSeconds;
	uint32 mn_timeoutSeconds;
	uint16 mn_port;
	std::string ms_ip;
};

class TcpSSendAmsg : public Amsg
{
public:
	explicit TcpSSendAmsg(uint32 uSrvID, uint32 uID, uint32 uSid, uint8 uMsgType, const std::string &strAddr, const std::string &strMsg)
		: Amsg(AmsgType::TCP_S_SEND)
		, mn_srvid(uSrvID)
		, mn_id(uID)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_sessionAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_sessionAddr;
	std::string ms_msg;
};

class TcpSCloseAmsg : public Amsg
{
public:
	explicit TcpSCloseAmsg(uint32 uSrvID, const std::string& strAddr)
		: Amsg(AmsgType::TCP_S_CLOSE)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	std::string ms_sessionAddr;
};

class TcpSCloseSafeAmsg : public Amsg
{
public:
	explicit TcpSCloseSafeAmsg(uint32 uSrvID, const std::string& strAddr)
		: Amsg(AmsgType::TCP_S_CLOSE_SAFE)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	std::string ms_sessionAddr;
};

class TcpSCloseAllAmsg : public Amsg
{
public:
	explicit TcpSCloseAllAmsg(uint32 uSrvID)
		: Amsg(AmsgType::TCP_S_CLOSE_ALL)
		, mn_srvid(uSrvID) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
};

class TcpSCloseAllSafeAmsg : public Amsg
{
public:
	explicit TcpSCloseAllSafeAmsg(uint32 uSrvID)
		: Amsg(AmsgType::TCP_S_CLOSE_ALL_SAFE)
		, mn_srvid(uSrvID) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
};

class TcpSSettestAmsg : public Amsg
{
public:
	explicit TcpSSettestAmsg(uint32 uSrvID, const std::string &strAddr, bool bPass)
		: Amsg(AmsgType::TCP_S_SET_TESTING)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr)
		, mb_pass(bPass) {}
	CANNOT_MARSHAL()
public:
	bool mb_pass;
	uint32 mn_srvid;
	const std::string ms_sessionAddr;
};

class TcpSSetActorAmsg : public Amsg
{
public:
	explicit TcpSSetActorAmsg(uint32 uSrvID, const std::string &strAddr, ActorPtr &pActor)
		: Amsg(AmsgType::TCP_S_SET_ACTOR)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr)
		, mp_actor(pActor) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	std::string ms_sessionAddr;
	ActorPtr mp_actor;
};

class TcpSStartUpCbAmsg : public Amsg
{
public:
	explicit TcpSStartUpCbAmsg(uint32 uSrvID)
		: Amsg(AmsgType::TCP_S_CB_STARTUP)
		, mn_srvid(uSrvID) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
};

class TcpSShutdownCbAmsg : public Amsg
{
public:
	explicit TcpSShutdownCbAmsg(uint32 uSrvID)
		: Amsg(AmsgType::TCP_S_CB_SHUTDOWN)
		, mn_srvid(uSrvID) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
};

class TcpSConnCbAmsg : public Amsg
{
public:
	explicit TcpSConnCbAmsg(uint32 uSrvID, const std::string &strAddr, const std::string &strRemoteIP)
		: Amsg(AmsgType::TCP_S_CB_CONNECTED)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr)
		, ms_remoteIP(strRemoteIP) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	std::string ms_sessionAddr;
	std::string ms_remoteIP;
};

class TcpSDisconnCbAmsg : public Amsg
{
public:
	explicit TcpSDisconnCbAmsg(uint32 uSrvID, const std::string &strAddr, bool bCloseByRemote, const std::string &strMsg)
		: Amsg(AmsgType::TCP_S_CB_DISCONNECTED)
		, mb_closeByRemote(bCloseByRemote)
		, mn_srvid(uSrvID)
		, ms_sessionAddr(strAddr)
		, ms_msg(strAddr) {}
	CANNOT_MARSHAL()
public:
	bool mb_closeByRemote;
	uint32 mn_srvid;
	std::string ms_sessionAddr;
	std::string ms_msg;
};

class TcpSRecvCbAmsg : public Amsg
{
public:
	explicit TcpSRecvCbAmsg(uint32 uSrvID, const std::string &strAddr,
		uint32 uID, uint32 uSid, uint8 uMsgType, const std::string &strMsg)
		: Amsg(AmsgType::TCP_S_CB_RECV_DATA)
		, mn_srvid(uSrvID)
		, mn_id(uID)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_sessionAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_sessionAddr;
	std::string ms_msg;
};

class TcpSRecvTestCbAmsg : public Amsg
{
public:
	explicit TcpSRecvTestCbAmsg(uint32 uSrvID, const std::string &strAddr, 
		uint32 uID, uint32 uSid, uint8 uMsgType, const std::string &strMsg)
		: Amsg(AmsgType::TCP_S_CB_RECV_TEST_DATA)
		, mn_srvid(uSrvID)
		, mn_id(uID)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_sessionAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_sessionAddr;
	std::string ms_msg;
};

class TcpSErrorCbAmsg : public Amsg
{
public:
	explicit TcpSErrorCbAmsg(uint32 uSrvID, const std::string& strMsg)
		: Amsg(AmsgType::TCP_S_CB_ERROR)
		, mn_srvid(uSrvID)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_srvid;
	std::string ms_msg;
};

////////////////////////////////////////////////////////////////////////
// tcp clt

class TcpCConnectAmsg : public Amsg
{
public:
	explicit TcpCConnectAmsg(const std::string &strIp, uint16 uPort)
		: Amsg(AmsgType::TCP_C_CONNECT)
		, mn_port(uPort)
		, ms_ip(strIp) {}
	CANNOT_MARSHAL()
public:
	uint16 mn_port;
	std::string ms_ip;
};

class TcpCSendAmsg : public Amsg
{
public:
	explicit TcpCSendAmsg(const std::string &strConnAddr, uint32 uID, uint32 uSid,
		uint8 uMsgType, const std::string &strMsg)
		: Amsg(AmsgType::TCP_C_SEND)
		, mn_id(uID)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_connAddr(strConnAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_connAddr;
	std::string ms_msg;
};

class TcpCCloseAmsg : public Amsg
{
public:
	explicit TcpCCloseAmsg()
		: Amsg(AmsgType::TCP_C_CLOSE) {}
};

class TcpCCloseSafeAmsg : public Amsg
{
public:
	explicit TcpCCloseSafeAmsg()
		: Amsg(AmsgType::TCP_C_CLOSE_SAFE) {}
};

class TcpCIpsReqAmsg : public Amsg
{
public:
	explicit TcpCIpsReqAmsg(const std::string &strUrl)
		: Amsg(AmsgType::TCP_C_IPS_REQ)
		, ms_url(strUrl) {}
	CANNOT_MARSHAL()
public:
	std::string ms_url;
};

class TcpCIpsResAmsg : public Amsg
{
public:
	explicit TcpCIpsResAmsg(std::vector<std::string> &vecIps)
		: Amsg(AmsgType::TCP_C_IPS_RES)
		, mo_ips(std::move(vecIps)) {}
	CANNOT_MARSHAL()
public:
	std::vector<std::string> mo_ips;
};

class TcpCConnCbAmsg : public Amsg
{
public:
	explicit TcpCConnCbAmsg(const std::string &strAddr)
		: Amsg(AmsgType::TCP_C_CB_CONNECTED)
		, ms_connAddr(strAddr) {}
	CANNOT_MARSHAL()
public:
	std::string ms_connAddr;
};

class TcpCConnFailCbAmsg : public Amsg
{
public:
	explicit TcpCConnFailCbAmsg(const std::string &strAddr, const std::string &strErrMsg)
		: Amsg(AmsgType::TCP_C_CB_CONNECT_FAIL)
		, ms_connAddr(strAddr)
		, ms_errmsg(strErrMsg) {}
	CANNOT_MARSHAL()
public:
	std::string ms_connAddr;
	std::string ms_errmsg;
};

class TcpCDissconnCbAmsg : public Amsg
{
public:
	explicit TcpCDissconnCbAmsg(const std::string &strAddr, const std::string &strMsg, bool bCloseByRemote)
		: Amsg(AmsgType::TCP_C_CB_DISCONNECTED)
		, mb_closedByRemote(bCloseByRemote)
		, ms_connAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	bool mb_closedByRemote;
	std::string ms_connAddr;
	std::string ms_msg;
};

class TcpCRecvCbAmsg : public Amsg
{
public:
	explicit TcpCRecvCbAmsg(const std::string &strAddr, uint32 uId, uint32 uSid,
		uint8 uMsgType, const std::string &strMsg)
		: Amsg(AmsgType::TCP_C_CB_RECV_DATA)
		, mn_id(uId)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_connAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_connAddr;
	std::string ms_msg;
};

class TcpCRecvTestCbAmsg : public Amsg
{
public:
	explicit TcpCRecvTestCbAmsg(const std::string &strAddr, uint32 uId, uint32 uSid,
		uint8 uMsgType, const std::string &strMsg)
		: Amsg(AmsgType::TCP_C_CB_RECV_TEST_DATA)
		, mn_id(uId)
		, mn_sid(uSid)
		, mn_msgtype(uMsgType)
		, ms_connAddr(strAddr)
		, ms_msg(strMsg) {}
	CANNOT_MARSHAL()
public:
	uint32 mn_id;
	uint32 mn_sid;
	uint8 mn_msgtype;
	std::string ms_connAddr;
	std::string ms_msg;
};

class TcpCErrorCbAmsg : public Amsg
{
public:
	explicit TcpCErrorCbAmsg(const std::string &strErrMsg)
		: Amsg(AmsgType::TCP_C_CB_ERROR)
		, ms_errmsg(strErrMsg) {}
	CANNOT_MARSHAL()
public:
	std::string ms_errmsg;
};

//////////////////////////////////////////////////////////////////////
// other

NamespaceEnd

#endif
