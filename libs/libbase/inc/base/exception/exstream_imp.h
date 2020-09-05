#ifndef LIBBASE_EXCEPTION_EXSTREAM_IMP_H_
#define LIBBASE_EXCEPTION_EXSTREAM_IMP_H_

#include "base/typedef.h"
#include "base/safe_singleton.h"
#include <boost/noncopyable.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/filesystem.hpp>
#include <sstream>
#include <string>
#include <cstdio>

NamespaceStart

class ExceptionStreamImp : boost::noncopyable, public SafeSingleton<ExceptionStreamImp>
{
public:
	explicit ExceptionStreamImp();

	~ExceptionStreamImp();

	bool config(const std::string& strDir, const std::string& strName, bool bAppend, uint32 uPageSize);

	bool write(const std::string& strFlag, const std::string& strMsg);

private:
	void flushData(const std::string& strFlag, const std::string& strMsg);

	bool firstOpenFile(const std::string& strPath, bool bAppend);

	bool backupFile();

	void closeFile();

private:
	FILE* mp_file;
	std::string ms_dir;
	std::string ms_name;
	std::string ms_curPath;
	std::size_t mn_pageSize;
	std::size_t mn_fileIndex;
	std::size_t mn_curSize;
	bool mb_append;
	bool mb_isStart;

	std::mutex mo_mutex;
};

NamespaceEnd

#endif
