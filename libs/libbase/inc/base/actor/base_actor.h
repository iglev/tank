#ifndef LIBBASE_BASE_ACTOR_H_
#define LIBBASE_BASE_ACTOR_H_

#include "base/typedef.h"
#include "base/exector/asio_svr.h"
#include "base/actor/base_actor_msg.h"
#include <boost/noncopyable.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <unordered_map>

#define ACTOR_CAST(T,p) ((T*)(p.get()))
#define ALEV_ACTOR_ROOT "root"

NamespaceStart

class Actor;
typedef std::shared_ptr<Actor> ActorPtr;

class Actor : boost::noncopyable
{
	typedef boost::asio::io_service::strand Channel;
	typedef std::weak_ptr<Actor> ActorWPtr;
	typedef std::unordered_map<std::string, ActorPtr> ActorPtrMap;
	typedef std::unordered_map<std::string, ActorWPtr> ActorWPtrMap;
	typedef std::unordered_map<std::string, std::shared_ptr<ActorWPtrMap> > ActorIdentMap;

public:
	explicit Actor(AsioSvc* pSvr, const std::string& strIdent, const std::string& strAddr);

	virtual ~Actor();

	std::string getIdent() const {
		return ms_ident;
	}

	std::string getAddr() const {
		return ms_addr;
	}

	AsioSvc* getSvr() const {
		return mp_svr;
	}

	boost::asio::io_service::strand& getChannel()
	{
		return mo_channel;
	}

	bool isValid() const {
		return mb_isValid.load(std::memory_order_acquire);
	}

	bool setRoute(const std::string& strRouteAddr) {
		ms_routeAddr = strRouteAddr;
		return true;
	}

	bool addActor(ActorPtr& pActor);

	bool addActorByList(std::vector<ActorPtr>& vecActor);

	bool removeActor(const std::string& strAddr);

	void removeAll();

	ActorPtr getActor(const std::string& strAddr);

	std::vector<ActorPtr> getActorByIdent(const std::string& strIdent);

	std::vector<ActorPtr> getAllActor();

	bool send(const std::string& strReceiver, const AmsgPtr& pMsg);

private:
	inline void sendIn(const ActorPtr& pReceiver, const std::string& strSender, const AmsgPtr& pMsg)
	{
		pReceiver->postIn(pReceiver, strSender, pMsg);
	}

	inline void postIn(const ActorPtr& pReceiver, const std::string& strSender, const AmsgPtr& pMsg)
	{
		mo_channel.post(std::bind(&Actor::processIn, pReceiver, strSender, pMsg));
	}

private:
	bool routeSend(const std::string& strSender, const std::string& strReceiver, const AmsgPtr& pMsg);

	void processIn(const std::string& strSender, const AmsgPtr& pMsg);

private:
	virtual void process(const std::string& strSender, const AmsgPtr& pMsg) = 0;

private:
	std::atomic<bool> mb_isValid;
	AsioSvc* mp_svr;
	Channel mo_channel;
	std::string ms_ident;
	std::string ms_addr;
	std::string ms_routeAddr;
	ActorPtrMap mo_mail;
	ActorIdentMap mo_identMail;

	std::mutex mo_mutex;
};

NamespaceEnd

#endif

