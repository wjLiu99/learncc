#include "tcp_server.h"
#include "logger.h"

#include <functional>
#include <strings.h>

static event_loop *check_loop (event_loop *loop) {
    if (nullptr == loop)
    {
        LOG_FATAL("%s:%s:%d mainLoop is null! \n", __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

tcp_server::tcp_server (event_loop *loop,
                        const inet_address &listen_addr,
                        const std::string &name,
                        option opt) :
                        loop_(check_loop(loop)),
                        ip_port_(listen_addr.to_ip_port()),
                        name_(name),
                        acceptor_(new acceptor(loop, listen_addr, opt == REUSE_PORT)),
                        threadpool_(new event_loop_threadpool(loop, name_)),
                        conn_cb_(),
                        msg_cb_(),
                        next_connid_(1),
                        started_(0) {
    // 设置新连接回调
    acceptor_->set_newconnection_cb(
        std::bind(&tcp_server::new_connection, this, std::placeholders::_1, std::placeholders::_2)
    );


}

tcp_server::~tcp_server () {
    for (auto &item : connections_) {
        // 有map中的智能指针拷贝构造，释放map中的智能指针
        tcp_connection_ptr conn(item.second);
        item.second.reset();

        conn->get_loop()->run_in_loop(
            std::bind(&tcp_connection::connect_destroyed, conn)
        );
    }
}

void tcp_server::set_threadnum (int num) {
    threadpool_->set_threadnum(num);
}

void tcp_server::start () {
    if (started_++ == 0) {
        //启动线程池
        threadpool_->start(thread_init_cb_);
        // start在mainloop中调用，调用listenfd的回调函数
        loop_->run_in_loop(std::bind(&acceptor::listen, acceptor_.get()));
    }
}

//给acceptor设置的新连接回调函数
void tcp_server::new_connection (int sockfd, const inet_address &remote_addr) {
    // 获取一个subloop
    event_loop *ioloop = threadpool_->get_nextloop();
    char buf[64] = {0};
    snprintf(buf, sizeof buf, "-%s#%d", ip_port_.c_str(), next_connid_++);
    std::string conn_name = name_ + buf;
    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s \n",
        name_.c_str(), conn_name.c_str(), remote_addr.to_ip_port().c_str());

    // 获取socket本地地址，用于构造tcpconnect对象
    sockaddr_in local;
    bzero(&local, sizeof local);
    socklen_t len = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &len) < 0)
    {
        LOG_ERROR("sockets::getLocalAddr");
    }
    inet_address local_addr(local);
    tcp_connection_ptr conn(new tcp_connection(
                            ioloop,
                            conn_name,
                            sockfd,
                            local_addr,
                            remote_addr
                            ));
    connections_[conn_name] = conn;
    
    // 给新连接设置好回调函数，tcpconnection中给channel设置回调函数，handle中会调用这些回调函数
    // channel就绪后会调用tcp connection的handle函数，handle会按事件调用这些用户设置的回调
    conn->set_connection_cb(conn_cb_);
    conn->set_message_cb(msg_cb_);
    conn->set_write_complete_cb(write_complete_cb_);
    conn->set_close_cb(
        std::bind(&tcp_server::remove_connection, this, std::placeholders::_1)
    );
    // 将channel添加到poller上，enable reading并调用conn cb
    ioloop->run_in_loop(std::bind(&tcp_connection::connect_established, conn));
}

// 要找到conn所在的线程进行删除
void tcp_server::remove_connection (const tcp_connection_ptr &conn) {
    loop_->run_in_loop(
        std::bind(&tcp_server::remove_connection_inloop, this, conn)
    );
}

void tcp_server::remove_connection_inloop (const tcp_connection_ptr &conn) {
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n", 
        name_.c_str(), conn->name().c_str());
    connections_.erase(conn->name());
    event_loop *ioloop = conn->get_loop();
    ioloop->queue_in_loop(
        std::bind(&tcp_connection::connect_destroyed, conn)
    );
}
