#ifndef _INRT_ADDR_H_
#define _INRT_ADDR_H_



#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>

// 封装socket地址类型
class inet_address
{
public:
    explicit inet_address(uint16_t port = 0, std::string ip = "127.0.0.1");
    explicit inet_address(const sockaddr_in &addr)
        : addr_(addr)
    {}

    std::string to_ip() const;
    uint16_t to_port() const;
    std::string to_ip_port() const;

    const sockaddr_in* get_sockaddr() const {return &addr_;}
    void set_sockaddr(const sockaddr_in &addr) { addr_ = addr; }
private:
    sockaddr_in addr_;
};

#endif