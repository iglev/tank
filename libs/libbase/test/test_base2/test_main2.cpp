#include <iostream>
#include <cstdio>
#include <string>
#include <cstring>
#include <cassert>
#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include "base/exception/extrack.h"
#include "base/exector/asio_svr.h"

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

int main(int argc, char* argv[]) {

	testAsio(argc, argv);

	return 0;
}


