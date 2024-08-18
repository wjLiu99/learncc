#ifndef _CSERVER_H_
#define _CSERVER_H_
#include <boost/asio.hpp>
#include "csession.h"
#include <memory.h>
#include <map>
#include <mutex>
using namespace std;
using boost::asio::ip::tcp;
class cserver
{
public:
	cserver(boost::asio::io_context& io_context, short port);
	~cserver();
	void clear_session(std::string);
private:
	// 处理连接请求
	void handle_accept(shared_ptr<csession>, const boost::system::error_code & error);
	// 开始监听
	void start_accept();
	boost::asio::io_context &io_context_;
	short port_;
	tcp::acceptor acceptor_;
	// 保存sessionid和session的映射关系
	std::map<std::string, shared_ptr<csession>> sessions_;
	std::mutex mtx_;
};

#endif