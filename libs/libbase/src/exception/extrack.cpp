#include "base/exception/extrack.h"
#include "base/exception/exstream_imp.h"

NamespaceUsing

Extrack::~Extrack()
{
	// 进程退出时，析构
	ExceptionStreamImp::release();
}

bool Extrack::config(const std::string strDir, const std::string& strName, bool bAppend, uint32 uPageSize)
{
	createInstance();
	return ExceptionStreamImp::ref().config(strDir, strName, bAppend, uPageSize);
}

bool Extrack::write(const std::string& strMark, const std::string& strData)
{
	createInstance();
	return ExceptionStreamImp::ref().write(strMark, strData);
}

