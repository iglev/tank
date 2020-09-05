#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cassert>
#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/exector/asio_svr.h"

#include "zmq.h"
#include "base/azmq/socket.hpp"
#include "boost/asio.hpp"
#include <array>

#ifdef _DEBUG
#include "vld.h"
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

int testZMQ(int argc, char* argv[]) {

	void* context = zmq_ctx_new();
	void* responder = zmq_socket(context, ZMQ_REP);
	int rc = zmq_bind(responder, "tcp://*:5555");
	assert(rc == 0);

	while (1) {
		char buffer[10];
		zmq_recv(responder, buffer, 10, 0);
		printf("Received Hello\n");
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		zmq_send(responder, "World", 5, 0);
	}
	return 0;
}

AsioSvc gSvc;

class azmqOne
{
public:
	void reqExec() {
		azmq::req_socket req(gSvc.getSvc());
		azmq::req_socket reqSocket(gSvc.getSvc());
		//reqSocket.set_option(azmq::socket::snd_timeo(2000));
		//reqSocket.set_option(azmq::socket::rcv_timeo(2000));
		reqSocket.connect("tcp://127.0.0.1:9999");

		while (!gSvc.isPressCtrlC()) {
			try {
				reqSocket.send(boost::asio::buffer("hello"));
			}
			catch (boost::system::system_error& e) {
				std::cout << "reqExec send: " << e.what() << std::endl;
			}
			
			std::array<char, 256> buf;
			std::size_t size = 0;
			try {
				size = reqSocket.receive(boost::asio::buffer(buf));
				std::cout << size << std::endl;
			}
			catch (boost::system::system_error& e) {
				std::cout << "reqExec receive: " << e.what() << std::endl;
			}

		}
	}

	void rspExec() {
		azmq::rep_socket reponseSocket(gSvc.getSvc());
		reponseSocket.bind("tcp://127.0.0.1:9999");
		std::array<char, 256> buf;
		while (!gSvc.isPressCtrlC()) {
			std::size_t size = 0;
			try {
				size = reponseSocket.receive(boost::asio::buffer(buf));
				std::cout << size << " " << std::string(buf.data(), size) << std::endl;
			}
			catch (boost::system::system_error& e) {
				std::cout << "rspExec receive: " << e.what() << std::endl;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(1000));

			try {
				reponseSocket.send(boost::asio::buffer(buf, size));
			}
			catch (boost::system::system_error& e) {
				std::cout << "rspExec send: " << e.what() << std::endl;
			}
		}
	}
};

int testAZMQ(int argc, char* argv[]) {
	gSvc.start();

	azmqOne one;
	std::thread t1(std::bind(&azmqOne::reqExec, &one));
	std::thread t2(std::bind(&azmqOne::rspExec, &one));

	while (!gSvc.isPressCtrlC()) {
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	t1.join();
	t2.join();
	gSvc.stop();
	return 0;
}

int main(int argc, char* argv[]) {

	//testAsio(argc, argv);
	//testZMQ(argc, argv);
	testAZMQ(argc, argv);

	return 0;
}


