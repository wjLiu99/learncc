#ifndef _CONNECTOR_H_
#define _CONNECTOR_H_

#include "noncopyable.h"
#include "inet_addr.h"

#include <functional>
#include <memory>
#include <atomic>
class channel;
class event_loop;

// 只负责与服务器进行连接，连接成功后将sockfd封装成channel监听写事件，触发后调用remove_and_reset_channel，将sockfd传给用户client的连接回调构造tcp connection，并调用用户设置的连接回调
class connector : noncopyable, public std::enable_shared_from_this<connector> {

public:
    typedef std::function<void (int sockfd)> new_connection_cb;
    connector (event_loop *loop, const inet_address &server_addr);
    ~connector ();

    void set_newconnection_cb (const new_connection_cb &cb) {
        conn_cb_ = cb;
    }

    void start ();  //可在任何线程中调用
    void restart ();//只能在loop线程中调用
    void stop ();   //可在任何线程调用

    const inet_address &server_address () const { return server_addr_; }
private:
    enum states {
        DISCONNECTED,
        CONNECTING,
        CONNECTED
    };
    // static const int max_retry_delay_ms = 30 * 1000;
    // static const int init_retry_delay_ms = 500;

    void set_state (states s) { state_ = s; }
    void start_inloop ();
    void stop_inloop ();
    void connect ();
    void connecting (int s);
    void handle_write ();
    void handle_error ();
    // void retry (int s);
    int remove_and_reset_channel (); //连接成功将sockfd返回给client构造tcp connection
    void reset_channel ();

    event_loop *loop_;
    inet_address server_addr_;
    std::atomic<bool> connect_;
    std::atomic<states> state_;
    std::unique_ptr<channel> channel_;

    new_connection_cb conn_cb_;
    int retry_delay_ms_;


};

#endif