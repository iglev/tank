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
	// Actor 内部协议id
	ACTOR_INNER_MSG_ROUTE     = 1, // route
	ACTOR_INNER_MSG_KILL_SELF = 2, // kill self
	ACTOR_INNER_MSG_ADD_ACTOR = 3, // add actor
	ACTOR_INNER_MSG_DEL_ACTOR = 4, // del actor

	ACTOR_INNER_MSG_GUARD = 1000,

	// Actor 外部协议id

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



//////////////////////////////////////////////////////////////////////
// other

NamespaceEnd

#endif
