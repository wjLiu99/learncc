#include "tcp_client.h"
#include "logger.h"
#include "connector.h"
#include "event_loop.h"
#include <strings.h>
static event_loop *check_loop (event_loop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("loop is null");
    }
    return loop;
}
void remove_connect (event_loop *loop, const tcp_connection_ptr &conn) {
    loop->queue_in_loop(
        std::bind(&tcp_connection::connect_destroyed, conn)
    );
}
void remove_connector(const connector_ptr& connector)
{
  //connector->
}

tcp_client::tcp_client (event_loop *loop, const inet_address &server_addr, const std::string &name) :
                        loop_(check_loop(loop)),
                        connector_(new connector(loop, server_addr)),
                        name_(name),
                        conn_cb_(),
                        msg_cb_(),
                        connect_(true),
                        next_connid_(1) {

    // 为connector设置连接回调函数,连接成功调用，使用本地地址和远端地址构造tcp connection 并调用用户设置的conn cb
    connector_->set_newconnection_cb(
        std::bind(&tcp_client::new_connection, this, std::placeholders::_1)
    );
    LOG_INFO("tcp client %s connector\n", name_.c_str());
}

tcp_client::~tcp_client () {
    LOG_INFO("tcp client ~ %s\n", name_.c_str());
    tcp_connection_ptr conn;
    bool unique = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        unique = connection_.unique();
        conn = connection_;
    }

    if (conn) {
        close_cb cb = std::bind(remove_connect, loop_, std::placeholders::_1);
        loop_->run_in_loop(
            std::bind(&tcp_connection::set_close_cb, conn, cb)
        );
        if (unique) {
            conn->force_close();
        }
    } else {
        connector_->stop();
        loop_->run_in_loop(
            std::bind(remove_connector, connector_)
        );

    }
}

// 连接服务器
void tcp_client::connect () {
    LOG_INFO("tcp client connect to %s\n", connector_->server_address().to_ip_port().c_str());

    connect_ = true;
    connector_->start();
}

// 断开连接
void tcp_client::disconnect () {
    connect_ = false;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (connection_) {
            connection_->shutdown();
        }
    }
}

void tcp_client::stop () {
    connect_ = false;
    connector_->stop();
}
// connector 完成连接监听读回调触发后调用，将连接好的sockfd传出构造tcp connection
void tcp_client::new_connection (int sockfd) {
    struct sockaddr_in remote;
    bzero(&remote, sizeof remote);
    socklen_t addrlen = sizeof remote;
    if (::getpeername(sockfd, (sockaddr *)(&remote), &addrlen) < 0)
    {
       LOG_ERROR("sockets::getPeerAddr");
    }
    inet_address remote_addr(remote);
    char buf[32] = {0};
    snprintf(buf, sizeof buf, ":%s#%d", remote_addr.to_ip_port().c_str(), next_connid_++);
    std::string conn_name = name_ + buf;

    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t len = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &len) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    inet_address local_addr(local);

    tcp_connection_ptr conn(new tcp_connection(loop_,
                                                conn_name,
                                                sockfd,
                                                local_addr,
                                                remote_addr));

    conn->set_connection_cb(conn_cb_);
    conn->set_message_cb(msg_cb_);
    conn->set_write_complete_cb(write_complete_cb_);
    conn->set_close_cb(
        std::bind(&tcp_client::remove_connection, this, std::placeholders::_1)
    );
    {
        std::lock_guard<std::mutex> lk(mtx_);
        connection_ = conn;
    }
    // 建立连接会调用conn cb
    conn->connect_established();

}

void tcp_client::remove_connection (const tcp_connection_ptr &conn) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        connection_.reset();
    }
    loop_->queue_in_loop(
        std::bind(&tcp_connection::connect_destroyed, conn)
    );

    if (connect_) {
        connector_->restart();
    }

}