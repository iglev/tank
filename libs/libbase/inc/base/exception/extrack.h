#ifndef LIBBASE_EXCEPTION_EXTRACK_H_
#define LIBBASE_EXCEPTION_EXTRACK_H_

#include "base/typedef.h"
#include "base/exception/trackinfo.h"
#include <boost/noncopyable.hpp>
#include <string>
#include <tuple>
#include <sstream>

#define LOGINFO(pre, ...) do{ \
		std::stringstream s; \
		printTuple(s, std::string(" "), std::make_tuple(__VA_ARGS__)); \
		Extrack::write(pre, std::move(s.str())); \
	} while(0)

#define LIBBASE_EXTRACK_DEFAULT_PAGE_SIZE (1024*1024*8)

NamespaceStart

class ExceptionStreamImp;
class Extrack : boost::noncopyable
{
public:
	/*
	* [config 设置输出的路径，文件名前缀 和 是否追加（第一次write接口之前调用，否则会无效）]
	* @param strDir		[输出目录]
	* @param strName	[文件名，不是路径，也不包含后缀]
	* @param bAppend	[true追加方式，false分文件方式]
	* @param uPageSize	[文件分页大小]
	* @return			[true成功，false失败]
	*/
	static bool config(const std::string strDir = "./", const std::string& strName = "error", bool bAppend = true, uint32 uPageSize = LIBBASE_EXTRACK_DEFAULT_PAGE_SIZE);

	/*
	* [write 写文件日志, 日志格式: #标识|日期时间] 日志信息]
	* 参考输出格式
	* [base]|20200816|connect failed -> TestClass:functionname, do_connect error dbid : 123
	* @param strMark	[日志标识]
	* @param strData	[日志内容]
	* @retrun			[true成功，false失败]
	* 
	*/
	static bool write(const std::string& strMark, const std::string& strData);

private:
	explicit Extrack() = default;
	~Extrack();
	Extrack(const Extrack&) = delete;
	Extrack(const Extrack&&) = delete;
	Extrack& operator=(const Extrack&) = delete;
	Extrack& operator=(const Extrack&&) = delete;

	/*
	* [createInstance 内部调用]
	*/
	static void createInstance() {
		static Extrack stObj;
	}
};

template<typename Stream, typename Tuple, std::size_t N>
struct PrintTupleDetail
{
	static void print(Stream& stStream, const std::string& strSpilt, const Tuple& stTuple) {
		PrintTupleDetail<Stream, Tuple, N - 1>::print(stStream, strSpilt, stTuple);
		stStream << strSpilt << std::get<N - 1>(stTuple);
	}
};

template<typename Stream, typename Tuple>
struct PrintTupleDetail<Stream, Tuple, 1>
{
	static void print(Stream& stStream, const std::string& strSplit, const Tuple& stTuple) {
		stStream << std::get<0>(stTuple);
	}
};

template<typename Stream, typename... Args>
std::string printTuple(Stream& stStream, const std::string& strSplit, const std::tuple<Args...>& stTuple) {
	PrintTupleDetail<Stream, decltype(stTuple), sizeof...(Args)>::print(stStream, strSplit, stTuple);
	return std::move(stStream.str());
}

NamespaceEnd

#endif
