#ifndef _CSERVER_H_
#define _CSERVER_H_
#include "comm.h"
#include <boost/asio.hpp>
                
namespace net = boost::asio;            
using tcp = boost::asio::ip::tcp; 

class cserver : public std::enable_shared_from_this<cserver> {
public:
    cserver(boost::asio::io_context &ioc, unsigned short &port);
    void start();

private:
    net::io_context &ioc_;
    tcp::acceptor acceptor_;
    
};
#endif