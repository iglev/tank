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
	* [config ���������·�����ļ���ǰ׺ �� �Ƿ�׷�ӣ���һ��write�ӿ�֮ǰ���ã��������Ч��]
	* @param strDir		[���Ŀ¼]
	* @param strName	[�ļ���������·����Ҳ��������׺]
	* @param bAppend	[true׷�ӷ�ʽ��false���ļ���ʽ]
	* @param uPageSize	[�ļ���ҳ��С]
	* @return			[true�ɹ���falseʧ��]
	*/
	static bool config(const std::string strDir = "./", const std::string& strName = "error", bool bAppend = true, uint32 uPageSize = LIBBASE_EXTRACK_DEFAULT_PAGE_SIZE);

	/*
	* [write д�ļ���־, ��־��ʽ: #��ʶ|����ʱ��] ��־��Ϣ]
	* �ο������ʽ
	* [base]|20200816|connect failed -> TestClass:functionname, do_connect error dbid : 123
	* @param strMark	[��־��ʶ]
	* @param strData	[��־����]
	* @retrun			[true�ɹ���falseʧ��]
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
	* [createInstance �ڲ�����]
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
