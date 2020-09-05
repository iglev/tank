#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cassert>
#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/exector/asio_svr.h"
#include "base/actor/base_actor.h"
#include "base/actor/base_actor_msg.h"

#include "boost/asio.hpp"
#include <array>

#ifdef _DEBUG
//#include "vld.h"
#endif

NamespaceUsing

void testAsioFunc(int iNum) {
	LOGINFO("TEST", iNum);
}

int testAsio(int argc, char *argv[]) {
	AsioSvc stSvc;
	stSvc.start();

	stSvc.getSvc().post(std::bind(testAsioFunc, 1234));
	while (!stSvc.isPressCtrlC()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
	stSvc.stop();

	return 0;

}

int testActorMsg(int argc, char* argv[]) {
	EchoMsgActorMsg stMsg(std::string("test msg"));
	std::string strData;
	stMsg.Marshal(strData);
	std::cout << strData << " size:" << strData.size() << std::endl;

	EchoMsgActorMsg stMsg2("");
	stMsg2.Unmarshal(strData);
	std::cout << stMsg2.mn_msgId << " " << stMsg2.ms_msg << std::endl;

	msgpack::object_handle obj = msgpack::unpack(strData.c_str(), strData.size());
	msgpack::type::tuple<int> tup;
	obj.get().convert(tup);
	std::cout << (ActorMsgType)(std::get<0>(tup)) << std::endl;

	return 0;
}


int testActorMsgBenchmark(int argc, char* argv[]) {
	double zn_start = clock();
	double zn_end;

	for (int i = 0; i < 100000; ++i) {
		EchoMsgActorMsg stMsg(std::string("test msg"));
		std::string strData;
		stMsg.Marshal(strData);
		//std::cout << strData << " size:" << strData.size() << std::endl;

		EchoMsgActorMsg stMsg2("");
		stMsg2.Unmarshal(strData);
		//std::cout << stMsg2.mn_msgId << " " << stMsg2.ms_msg << std::endl;
	}

	zn_end = clock();
	std::cout << (zn_end - zn_start) / CLOCKS_PER_SEC << std::endl;

	return 0;
}

int main(int argc, char* argv[]) {

	//testAsio(argc, argv);
	//testActorMsg(argc, argv);
	testActorMsgBenchmark(argc, argv);

	return 0;
}


