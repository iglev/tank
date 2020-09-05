#ifndef LIBBASE_ACTOR_MSG_H_
#define LIBBASE_ACTOR_MSG_H_

#include "base/typedef.h"
#include <msgpack.hpp>
#include <string>
#include <sstream>
#include <tuple>
#include <vector>
#include <memory>

NamespaceStart

enum ActorMsgType
{
	// Actor 内部协议id
	ACTOR_INNER_MSG_ROUTE     = 1, // route
	ACTOR_INNER_MSG_KILL_SELF = 2, // kill self
	ACTOR_INNER_MSG_ADD_ACTOR = 3, // add actor
	ACTOR_INNER_MSG_DEL_ACTOR = 4, // del actor

	ACTOR_INNER_MSG_GUARD = 1000,

	// Actor 外部协议id

	ECHO_MSG = 1001, // echo msg
};

class ActorMsg
{
public:
	ActorMsg(ActorMsgType iMsgId): mn_msgId(iMsgId) {}

	virtual void Marshal(std::string& strData) = 0;
	virtual void Unmarshal(const std::string& strData) = 0;
	virtual bool IsCanMarshal() = 0;

	template<typename ...T>
	void MarshalIn(std::string &strData, T&& ...args)
	{
		std::stringstream buffer;
		msgpack::pack(buffer, msgpack::type::make_tuple(std::forward<T>(args)...));
		strData = buffer.str();
	}

	template<typename ...T>
	void UnmarshalIn(const std::string& strData, T&& ...args)
	{
		msgpack::object_handle obj = msgpack::unpack(strData.c_str(), strData.size());
		auto tup = msgpack::type::make_tuple(std::forward<T>(args)...);
		obj.get().convert(tup);
		std::tie(std::forward<T>(args)...) = std::move(tup);
	}

public:
	int32 mn_msgId;
};

class Actor;
typedef std::shared_ptr<Actor> ActorPtr;
typedef std::shared_ptr<ActorMsg> ActorMsgPtr;

#define CAN_MARSHAL(...) void Marshal(std::string& strData) { \
	MarshalIn(strData, mn_msgId, __VA_ARGS__); } \
	void Unmarshal(const std::string& strData) { \
	UnmarshalIn(strData, mn_msgId, __VA_ARGS__); } \
	inline bool IsCanMarshal() { return true; }

#define CANOT_MARSHAL(...) void Marshal(std::string& strData) {} \
	void Unmarshal(const std::string& strData) {} \
	inline bool IsCanMarshal() { return false; }


//////////////////////////////////////////////////////////////////////
// echo msg

class EchoMsgActorMsg : public ActorMsg
{
public:
	explicit EchoMsgActorMsg(const std::string& strMsg)
		: ActorMsg(ActorMsgType::ECHO_MSG)
		, ms_msg(strMsg) {}
	CAN_MARSHAL(ms_msg)
public:
	std::string ms_msg;
};

//////////////////////////////////////////////////////////////////////
// inner

class RouteActorMsg : public ActorMsg
{
public:
	explicit RouteActorMsg(const std::string &strSender, const std::string strReceiver, const ActorMsgPtr &pMsg)
		: ActorMsg(ActorMsgType::ACTOR_INNER_MSG_ROUTE)
		, ms_sender(strSender)
		, ms_receiver(strReceiver)
		, mp_msg(pMsg) {}
	CANOT_MARSHAL()
public:
	std::string ms_sender;
	std::string ms_receiver;
	ActorMsgPtr mp_msg;
};

//////////////////////////////////////////////////////////////////////
// other

NamespaceEnd

#endif
