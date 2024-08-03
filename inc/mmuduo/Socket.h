#ifndef _SOCKET_H_
#define _SOCKET_H_
#include "noncopyable.h"
class inet_address;

class Socket : noncopyable {

public:
    explicit Socket (int s) : sockfd_(s) {}
    ~Socket ();

    int fd () const { return sockfd_; }
    void bind_address (const inet_address &local_addr);
    void listen ();
    int accept (inet_address *remote_addr);
    int connect (const inet_address &server_addr);
    void close ();
    void shutdown_write ();

    void set_tcp_nodelay (bool on);
    void set_reuse_addr (bool on);
    void set_reuse_port (bool on);
    void set_keepalive (bool on);


private:

    const int sockfd_;
};

#endif