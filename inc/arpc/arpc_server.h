#ifndef _ARPC_SERVER_H_
#define _ARPC_SERVER_H_

#include <google/protobuf/service.h>
#include <google/protobuf/descriptor.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpConnection.h>
#include <muduo/net/EventLoop.h>
#include "tcp_server.h"
#include <string>
#include <functional>
#include <unordered_map>

#define LOG_LEVEL INFO
class arpc_server {

public:

    void register_service(::google::protobuf::Service*);
    void start();
    void set_thread_num(int num)
    {
        thread_num_ = num;
    }
private:

    // muduo::net::EventLoop loop_;
    event_loop loop_;
    int thread_num_ = 0;

    struct service_info {
        google::protobuf::Service *service_;
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor *> method_map_;
    };
    struct response_info {
        uint32_t id;
        google::protobuf::Message *response;
    };

    std::unordered_map<std::string, service_info> service_map_;

    // void on_connection (const muduo::net::TcpConnectionPtr &);
    // void on_message (const muduo::net::TcpConnectionPtr &, muduo::net::Buffer*, muduo::Timestamp);
    // void send_response (const muduo::net::TcpConnectionPtr &,response_info response_info);

    
    void on_connection (const tcp_connection_ptr &);
    void on_message (const tcp_connection_ptr &, buffer *, timestamp);
    void send_response (const tcp_connection_ptr &,response_info response_info);


};


#endif