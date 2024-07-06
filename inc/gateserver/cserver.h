#ifndef _CSERVER_H_
#define _CSERVER_H_
#include "comm.h"


class cserver : public std::enable_shared_from_this<cserver> {
public:
    cserver(boost::asio::io_context &ioc, unsigned short &port);
    void start();

private:
    tcp::acceptor acceptor_;
    net::io_context &ioc_;
    tcp::socket socket_;
};
#endif