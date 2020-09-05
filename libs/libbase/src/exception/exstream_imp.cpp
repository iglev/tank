#include <iostream>
#include <ctime>
#include "base/exception/exstream_imp.h"
#include "base/exception/extrack.h"

#define LIBBASE_DEFAULT_DIR "./" // 默认目录
#define LIBBASE_DEFAULT_FILENAME_PREFIX "error" // 默认文件名
#define LIBBASE_DEFAULT_FILENAME_SUFFIX ".log" // 默认后缀名

NamespaceStart

///////////////////////////////////////////////////////////////////////////
// public

ExceptionStreamImp::ExceptionStreamImp()
	: mp_file(nullptr)
	, ms_dir(LIBBASE_DEFAULT_DIR)
	, ms_name(LIBBASE_DEFAULT_FILENAME_PREFIX)
	, ms_curPath(std::string(LIBBASE_DEFAULT_DIR) + std::string(LIBBASE_DEFAULT_FILENAME_PREFIX) + std::string(LIBBASE_DEFAULT_FILENAME_SUFFIX))
	, mn_pageSize(LIBBASE_EXTRACK_DEFAULT_PAGE_SIZE)
	, mn_fileIndex(0)
	, mn_curSize(0)
	, mb_append(true)
	, mb_isStart(false)
{

}

ExceptionStreamImp::~ExceptionStreamImp()
{
	this->closeFile();
}

bool ExceptionStreamImp::config(const std::string& strDir, const std::string& strName, bool bAppend, uint32 uPageSize)
{
	std::lock_guard<std::mutex> lock(mo_mutex);

	if (mb_isStart) {
		return false;
	}

	try {
		boost::filesystem::path stFilePath = strDir;
		boost::filesystem::path stFullFilePath = boost::filesystem::system_complete(stFilePath);
		if (!boost::filesystem::exists(stFullFilePath)) {
			boost::filesystem::create_directories(stFullFilePath);
		}
		stFilePath = stFullFilePath / strName;
		std::stringstream stStream;
		stStream << stFilePath.string() << LIBBASE_DEFAULT_FILENAME_SUFFIX;
		std::string strNewPath = boost::filesystem::system_complete(stStream.str()).string();
		if (!firstOpenFile(strNewPath, bAppend)) {
			//路径有问题吧，无法创建
			std::cerr << "ExceptionStreamImp::config firstopenfile error : " << strNewPath
				<< " by " << (bAppend ? "append" : "write") << std::endl;
			return false;
		}

		mb_isStart = true;
		ms_dir = strDir;
		ms_name = strName;
		mb_append = bAppend;
		ms_curPath = strNewPath;
		mn_pageSize = uPageSize;
		if (mn_pageSize < 1024) {
			mn_pageSize = 1024;
		}
	}
	catch (std::exception& e) {
		std::cerr << "ExceptionStreamImp::set_path error : " << e.what() << std::endl;
		return false;
	}
	catch (...) {
		return false;
	}
	return true;
}

bool ExceptionStreamImp::write(const std::string& strFlag, const std::string& strMsg)
{
	std::lock_guard<std::mutex> lock(mo_mutex);
	
	std::size_t length = strMsg.length() + strFlag.length();
	if (length <= 0) {
		return true;
	}
	if (!mb_isStart) {
		if (!firstOpenFile(ms_curPath, mb_append)) {
			//路径有问题吧，无法创建
			std::cerr << "ExceptionStreamImp::write firstopenfile error : " << ms_curPath
				<< " by " << (mb_append ? "append" : "write") << std::endl;
			return false;
		}
		mb_isStart = true;
	}

	// 检查日志数据的大小
	if (mn_pageSize < length) {
		// 舍弃，一次日志超过单页上限，丢弃
		std::cerr << "ExceptionStreamImp::write data to large : " << length << " byte." << std::endl;
		return false;
	}

	// 检查文件是否可以放得下当前数据
	if ((mn_curSize + length) <= mn_pageSize) {
		flushData(strFlag, strMsg);
	} else {
		++mn_fileIndex;
		if (backupFile()) {
			flushData(strFlag, strMsg);
		} else {
			std::cerr << "ExceptionStreamImp::write backup file error, data discard." << std::endl;
			return false;
		}
	}
	return true;
}


///////////////////////////////////////////////////////////////////////////
// private

void ExceptionStreamImp::flushData(const std::string& strFlag, const std::string& strMsg)
{
	if (mp_file == nullptr) {
		std::cerr << "ExceptionStreamImp::flushdata error, no file handle." << std::endl;
		return;
	}

	struct tm t;
	std::time_t ts = std::time(nullptr);
	localtime_s(&t, &ts);
	char vecTime[32];
	std::memset(vecTime, '\0', sizeof(vecTime));
	std::strftime(vecTime, sizeof(vecTime), "%y%m%d %H:%M:%S", &t);

	// flush data
	fprintf(mp_file, "#%s|%s| %s\n", strFlag.c_str(), vecTime, strMsg.c_str());
	mn_curSize += strFlag.length() + strMsg.length() + 20;
	::fflush(mp_file);
}

bool ExceptionStreamImp::firstOpenFile(const std::string& strPath, bool bAppend)
{
	if (mb_append) {
		::errno_t iRet = fopen_s(&mp_file, strPath.c_str(), "a+");
		if (iRet != 0) {
			std::cerr << "ExceptionStreamImp::firstopenfile open fail, " << strPath << std::endl;
			return false;
		}
		if (mp_file != nullptr) {
			fseek(mp_file, 0, SEEK_END);
			mn_curSize = ftell(mp_file);
		}
	}
	else {
		::errno_t iRet = fopen_s(&mp_file, strPath.c_str(), "w");
		if (iRet != 0) {
			std::cerr << "ExceptionStreamImp::firstopenfile open fail, " << strPath << std::endl;
			return false;
		}
		mn_curSize = 0;
	}

	if (!mp_file) {
		//路径有问题吧，无法创建
		std::cerr << "ExceptionStreamImp::firstopenfile open path error : " << strPath << std::endl;
		return false;
	}

	return true;
}

bool ExceptionStreamImp::backupFile()
{
	// 关闭当前文件
	this->closeFile();

	boost::filesystem::path stFilePath = ms_dir;
	stFilePath = stFilePath / ms_name;
	try {
		std::stringstream stStream;

		// 查找一个新的文件名
		std::size_t tmpFileCnt = mn_fileIndex;
		for (std::size_t n = 0; n < 99999; ++n) {
			stStream.str("");
			stStream << stFilePath.string() << "_" << tmpFileCnt << LIBBASE_DEFAULT_FILENAME_SUFFIX;

			std::string fileName = boost::filesystem::system_complete(stStream.str()).string();
			if (!boost::filesystem::exists(fileName)) {
				// 文件不存在
				if (boost::filesystem::exists(ms_curPath)) {
					// 将当前文件改名为新的文件
					boost::filesystem::rename(ms_curPath, fileName);
				}

				::errno_t iRet = fopen_s(&mp_file, ms_curPath.c_str(), "a+");
				if (iRet != 0 || mp_file == nullptr) {
					std::cerr << "ExceptionStreamImp::backupfile can not open [" << ms_curPath << "]." << std::endl;
					return false;
				}
				mn_curSize = 0;
				mn_fileIndex = tmpFileCnt;
				break;
			}
			++tmpFileCnt;
		}
	}
	catch (std::exception& e) {
		std::cerr << "ExceptionStreamImp::backupfile error : " << e.what() << std::endl;
		return false;
	}
	catch (...) {
		return false;
	}
	return true;
}

void ExceptionStreamImp::closeFile()
{
	if (mp_file != nullptr) {
		::fflush(mp_file);
		fclose(mp_file);
		mp_file = nullptr;
	}
}


NamespaceEnd

