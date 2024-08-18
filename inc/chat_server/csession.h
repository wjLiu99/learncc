#ifndef _CSESSION_H_
#define _CSESSION_H_
#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>
// #include <boost/beast/http.hpp>
// #include <boost/beast.hpp>
#include <boost/asio.hpp>

#include "comm.h"
#include "msg_node.h"
using namespace std;


// namespace beast = boost::beast;         // from <boost/beast.hpp>
// namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>


class cserver;
class logic_system;

class csession: public std::enable_shared_from_this<csession>
{
public:
	csession(boost::asio::io_context& io_context, cserver* server);
	~csession();
	tcp::socket& get_socket();
	std::string& get_session_id();
	void set_user_id (int uid);
	int get_user_id ();
	void start();
	void send(char* msg,  short max_length, short msgid);
	void send(std::string msg, short msgid);
	void close();
	std::shared_ptr<csession> shared_self();
	void async_read_body(int length);
	void async_read_head(int total_len);
private:
	void async_read_full(std::size_t maxLength, std::function<void(const boost::system::error_code& , std::size_t)> handler);
	void async_read_len(std::size_t  read_len, std::size_t total_len,
		std::function<void(const boost::system::error_code&, std::size_t)> handler);
	
	
	void handle_write(const boost::system::error_code& error, std::shared_ptr<csession> shared_self);
	tcp::socket socket_;
	std::string session_id_;
	char data_[MAX_LENGTH];
	cserver* server_;
	bool b_close_;
	// 发送队列
	std::queue<shared_ptr<send_node> > send_que_;
	std::mutex send_lock_;
	//收到的消息结构
	std::shared_ptr<recv_node> recv_msg_node_;
	// 头部是否解析完成
	bool _b_head_parse;
	//收到的头部结构
	std::shared_ptr<msg_node> recv_head_node_;

	int user_id_;
};
// 将接收的消息体构造成logic_node投递到logic system的消息队列中等待处理
class logic_node {
	friend class logic_system;
public:
	logic_node(shared_ptr<csession>, shared_ptr<recv_node>);
private:
	shared_ptr<csession> session_;
	shared_ptr<recv_node> recv_node_;
};


#endif