#ifndef _HTTP_CONN_H_
#define _HTTP_CONN_H_
#include "comm.h"
#include <boost/beast/http.hpp>
#include <boost/beast.hpp>
#include <boost/asio.hpp>

namespace beast = boost::beast;         
namespace http = beast::http;           
namespace net = boost::asio;            
using tcp = boost::asio::ip::tcp; 

class logic_system;
class http_conn :public std::enable_shared_from_this<http_conn> {
    friend class logic_system;
public:
    http_conn(boost::asio::io_context &ioc);
    void start();
    tcp::socket &get_socket () {
        return socket_;
    }
private:
    void parse_getparam();
    void check_tmo();   //检测是否超时
    void write_response();//响应
    void req_handler(); //处理请求
    tcp::socket socket_;
    beast::flat_buffer buffer_{8192};
    http::request<http::dynamic_body> req_;
    http::response<http::dynamic_body> res_;
    net::steady_timer tmo_{socket_.get_executor(), std::chrono::seconds(60)}; //定时器绑定到io调度器上

    std::string get_url_;
    std::unordered_map<std::string, std::string> get_params_;
};
#endif