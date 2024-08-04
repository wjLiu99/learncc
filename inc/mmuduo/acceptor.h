#ifndef _ACCEPTOR_H_
#define _ACCEPTOR_H_
#include "noncopyable.h"
#include "Socket.h"
#include "channel.h"

#include <functional>
class event_loop;
class inet_address;

class  acceptor : noncopyable {

public:
    typedef std::function<void(int s, const inet_address&)> new_connection_cb;
    acceptor (event_loop *loop, const inet_address &listen_addr, bool reuseport);
    ~acceptor ();

    void set_newconnection_cb (const new_connection_cb &cb) {
        new_conn_cb_ = cb;
    }

    bool listenning () const { return listenning_; }
    void listen ();


private:
    //acceptor读回调
    void handle_read ();
    event_loop *loop_;
    Socket accept_socket_;
    channel accept_channel_;
    //新连接回调函数
    new_connection_cb new_conn_cb_;
    bool listenning_;


};
#endif