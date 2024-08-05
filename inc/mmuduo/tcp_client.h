#ifndef _TCP_CLIENT_H_
#define _TCP_CLIENT_H_

#include "tcp_connection.h"
#include <mutex>

class connector;
typedef std::shared_ptr<connector> connector_ptr;

class tcp_client : noncopyable {

public:
    tcp_client (event_loop *loop, const inet_address &server_addr, const std::string &name);
    ~tcp_client ();

    void connect ();
    void disconnect ();
    void stop ();

    tcp_connection_ptr connection () const {
        std::lock_guard<std::mutex> lk(mtx_);
        return connection_;
    }

    event_loop *get_loop () const { return loop_; }
    const std::string &name () const { return name_; }

    void set_connection_cb (connection_cb cb) {
        conn_cb_ = std::move(cb);
    }
    void set_message_cb (message_cb cb) {
        msg_cb_ = std::move(cb);
    }

    void set_write_complete_cb (write_complete_cb cb) {
        write_complete_cb_ = std::move(cb);
    } 


private:
    void new_connection (int sockfd);
    void remove_connection (const tcp_connection_ptr &conn);
    
    event_loop *loop_;
    connector_ptr connector_;
    const std::string name_;
    connection_cb conn_cb_;
    message_cb msg_cb_;
    write_complete_cb write_complete_cb_;
    std::atomic<bool> connect_;
    int next_connid_;
    mutable std::mutex mtx_;
    tcp_connection_ptr connection_;
};
#endif