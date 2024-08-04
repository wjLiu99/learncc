#ifndef _TCP_SERVER_H_
#define _TCP_SERVER_H_
#include "event_loop.h"
#include "acceptor.h"
#include "inet_addr.h"
#include "noncopyable.h"
#include "eventloop_threadpool.h"
#include "callbacks.h"
#include "buffer.h"
#include "tcp_connection.h"
#include "event_loop.h"

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

class tcp_server : noncopyable {

public:
    typedef std::function<void(event_loop *)> thread_init_cb;
    enum option {
        NO_REUSE_PORT,
        REUSE_PORT,
    };

    tcp_server (event_loop *loop, const inet_address &listen_addr, const std::string &name, option option = NO_REUSE_PORT);
    ~ tcp_server ();

    void set_thread_init_cb(const thread_init_cb &cb) { thread_init_cb_ = cb; }
    void set_connection_cb(const connection_cb &cb) { conn_cb_ = cb; }
    void set_message_cb(const message_cb &cb) { msg_cb_ = cb; }
    void set_write_complete_cb(const write_complete_cb &cb) { write_complete_cb_ = cb; }

    // 设置底层subloop的个数
    void set_threadnum(int num);

    // 开启服务器监听
    void start();
private:
    void new_connection (int sockfd, const inet_address &remote_addr);
    void remove_connection (const tcp_connection_ptr &conn);
    void remove_connection_inloop (const tcp_connection_ptr &conn);


    typedef std::unordered_map<std::string, tcp_connection_ptr> connection_map;

    event_loop *loop_;
    const std::string ip_port_;
    const std::string name_;

    std::unique_ptr<acceptor> acceptor_;
    std::shared_ptr<event_loop_threadpool> threadpool_;

    connection_cb conn_cb_;
    message_cb msg_cb_;
    write_complete_cb write_complete_cb_;
    thread_init_cb thread_init_cb_;

    std::atomic_int started_;
    int next_connid_;
    connection_map connections_; //管理所有连接


};

#endif