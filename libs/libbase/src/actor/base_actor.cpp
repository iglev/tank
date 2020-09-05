#include "base/actor/base_actor.h"

NamespaceUsing

Actor::Actor(AsioSvc* pSvr, const std::string& strIdent, const std::string& strAddr)
	: mb_isValid(true)
	, mp_svr(pSvr)
	, mo_channel(mp_svr->getSvc())
	, ms_ident(strIdent)
	, ms_addr(strAddr)
	, ms_routeAddr("")
{

}

Actor::~Actor()
{
	removeAll();
}

bool Actor::addActor(ActorPtr& pActor)
{
	if (!pActor) {
		return false;
	}

	std::unique_lock<std::mutex> lock(mo_mutex);

	auto iterActor = mo_mail.find(pActor->getAddr());
	if (iterActor != mo_mail.end()) {
		// already exists
		return true;
	}

	// add actor
	mo_mail.insert(std::make_pair(pActor->getAddr(), pActor));

	// add into ident map
	auto iterIdentMap = mo_identMail.find(pActor->getIdent());
	if (iterIdentMap == mo_identMail.end()) {
		// new ident weak map
		std::shared_ptr<ActorWPtrMap> pMap;
		try {
			pMap = std::shared_ptr<ActorWPtrMap>(new ActorWPtrMap());
		}
		catch (...) {
			pMap.reset();
			return false;
		}
		pMap->insert(std::make_pair(pActor->getAddr(), pActor));
		mo_identMail.insert(std::make_pair(pActor->getIdent(), pMap));
		return true;
	}
	iterIdentMap->second->insert(std::make_pair(pActor->getAddr(), pActor));
	return true;
}

bool Actor::addActorByList(std::vector<ActorPtr>& vecActor)
{
	for (auto iter = vecActor.begin(); iter != vecActor.end(); ++iter) {
		addActor(*iter);
	}
	return true;
}

bool Actor::removeActor(const std::string& strAddr)
{
	if (strAddr.empty()) {
		return false;
	}

	std::unique_lock<std::mutex> lock(mo_mutex);

	auto iterActor = mo_mail.find(strAddr);
	if (iterActor == mo_mail.end()) {
		// not exists
		return false;
	}

	auto iterIdentMap = mo_identMail.find(iterActor->second->getIdent());
	if (iterIdentMap != mo_identMail.end()) {
		iterIdentMap->second->erase(iterActor->second->getAddr());
		if (iterIdentMap->second->size() <= 0) {
			mo_identMail.erase(iterIdentMap);
		}
	}
	mo_mail.erase(iterActor);
	return true;
}

void Actor::removeAll()
{
	std::unique_lock<std::mutex> lock(mo_mutex);
	mo_mail.clear();
	mo_identMail.clear();
}

ActorPtr Actor::getActor(const std::string& strAddr)
{
	std::unique_lock<std::mutex> lock(mo_mutex);
	
	auto iterActor = mo_mail.find(strAddr);
	if (iterActor == mo_mail.end()) {
		return ActorPtr();
	}
	return iterActor->second;
}

std::vector<ActorPtr> Actor::getActorByIdent(const std::string& strIdent)
{
	std::unique_lock<std::mutex> lock(mo_mutex);

	std::vector<ActorPtr> vecRes;
	auto iterIdentMap = mo_identMail.find(strIdent);
	if (iterIdentMap != mo_identMail.end()) {
		for (auto iter = iterIdentMap->second->begin(); iter != iterIdentMap->second->end(); ++iter) {
			auto tmp = iter->second.lock();
			if (tmp) {
				vecRes.push_back(tmp);
			}
		}
	}
	return std::move(vecRes);
}

std::vector<ActorPtr> Actor::getAllActor()
{
	std::unique_lock<std::mutex> lock(mo_mutex);

	std::vector<ActorPtr> vecRes;
	for (auto iter = mo_mail.begin(); iter != mo_mail.end(); ++iter)
	{
		vecRes.push_back(iter->second);
	}
	return std::move(vecRes);
}

bool Actor::send(const std::string& strReceiver, const AmsgPtr& pMsg)
{
	if (strReceiver.empty() || !pMsg) {
		return false;
	}
	return routeSend(ms_addr, strReceiver, pMsg);
}

///////////////////////////////////////////////////////////////////
// private

bool Actor::routeSend(const std::string& strSender, const std::string& strReceiver, const AmsgPtr& pMsg)
{
	std::unique_lock<std::mutex> lock(mo_mutex);

	auto iterActor = mo_mail.find(strReceiver);
	if (iterActor == mo_mail.end()) {
		auto iterRouteActor = mo_mail.find(ms_routeAddr);
		if (iterRouteActor == mo_mail.end()) {
			// not found route actor
			return false;
		}
		auto pRouteMsg = AmsgPtr(new RouteAmsg(strSender, strReceiver, pMsg));
		sendIn(iterRouteActor->second, strSender, pRouteMsg);
	}
	sendIn(iterActor->second, strSender, pMsg);
	return true;
}

void Actor::processIn(const std::string& strSender, const AmsgPtr& pMsg)
{
	if (!pMsg) {
		return;
	}

	auto iMsgId = pMsg->mn_msgId;
	if (iMsgId > AmsgType::ACTOR_INNER_MSG_GUARD) {
		this->process(strSender, pMsg);
		return;
	}

	switch (iMsgId) {
	case AmsgType::ACTOR_INNER_MSG_KILL_SELF:
	{
		removeAll();
		mb_isValid.store(false, std::memory_order_release);
		break;
	}
	case AmsgType::ACTOR_INNER_MSG_ROUTE:
	{
		// route msg
		RouteAmsg* pRouteMsg = (RouteAmsg*)(pMsg.get());
		routeSend(pRouteMsg->ms_sender, pRouteMsg->ms_receiver, pRouteMsg->mp_msg);
		break;
	}
	case AmsgType::ACTOR_INNER_MSG_ADD_ACTOR:
	{
		// add actor
		AddActorAmsg* pAddMsg = (AddActorAmsg*)(pMsg.get());
		addActorByList(pAddMsg->mo_list);
		break;
	}
	case AmsgType::ACTOR_INNER_MSG_DEL_ACTOR:
	{
		// del actor
		DelActorAmsg* pDelMsg = (DelActorAmsg*)(pMsg.get());
		for (auto iter = pDelMsg->mo_list.begin(); iter != pDelMsg->mo_list.end(); ++iter) {
			removeActor(*iter);
		}
		break;
	}
	default:
		break;
	}
}



