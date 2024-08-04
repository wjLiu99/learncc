#ifndef _TCP_CONNECTION_H_
#define _TCP_CONNECTION_H_
#include "noncopyable.h"
#include "inet_addr.h"
#include "callbacks.h"
#include "buffer.h"
#include "timestamp.h"
#include <memory>
#include <string>
#include <atomic>

class channel;
class event_loop;
class Socket;

class tcp_connection : noncopyable, public std::enable_shared_from_this<tcp_connection> {

public:
    tcp_connection (event_loop *loop, 
                    const std::string &name,
                    int sockfd,
                    const inet_address &local,
                    const inet_address &remote);
    ~tcp_connection ();

    event_loop *get_loop () const { return loop_; }
    const std::string &name () const { return name_; }
    const inet_address &local_address () const { return local_; }
    const inet_address &remote_address () const { return remote_; }

    bool connected () const { return state_ == CONNECTED; }

    void send (const std::string &buf);
    void shutdown ();

    void set_connection_cb(const connection_cb& cb)
    { conn_cb_ = cb; }

    void set_message_cb(const message_cb& cb)
    { msg_cb_ = cb; }

    void set_write_complete_cb(const write_complete_cb& cb)
    { write_comp_cb_ = cb; }

    void set_high_watermark_cb(const high_watermark_cb& cb, size_t highWaterMark)
    { high_watermark_cb_ = cb; high_watermark_ = highWaterMark; }

    void set_close_cb(const close_cb& cb)
    { close_cb_ = cb; }

    void connect_established ();
    void connect_destroyed ();
    void force_close ();
    void force_close_inloop ();
    


private:
    enum stateE {
        DISCONNECTED,
        CONNECTING,
        CONNECTED,
        DISCONNECTING
    };

    void set_state (stateE state) { state_ = state; }

    void handle_read (timestamp recv_time);
    void handle_write ();
    void handle_close ();
    void handle_error ();

    void send_inloop (const void *msg, size_t len);
    void shutdown_inloop ();


    event_loop *loop_;  //tcpconnection所属的subloop
    const std::string name_;
    std::atomic_int state_; //连接状态
    bool reading_;
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<channel> channel_;
    const inet_address local_;
    const inet_address remote_;

    connection_cb conn_cb_;
    message_cb msg_cb_;
    write_complete_cb write_comp_cb_;
    high_watermark_cb high_watermark_cb_;
    close_cb close_cb_;
    size_t high_watermark_;

    buffer ibuf_;
    buffer obuf_;

};
#endif