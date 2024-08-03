#include "Socket.h"
#include "inet_addr.h"

#include "logger.h"
#include <unistd.h>
#include <strings.h>
#include <netinet/tcp.h>

Socket::~Socket () {
    ::close(sockfd_);
}

void Socket::bind_address (const inet_address &local) {
    if (0 != ::bind(sockfd_, (struct sockaddr *)local.get_sockaddr(), sizeof(sockaddr_in))) {
        LOG_FATAL("bind sockfd : %d failed. ip:port : %s\n", sockfd_, local.to_ip_port().c_str());
    }


}

void Socket::listen () {
    if (0 != ::listen(sockfd_, 1024)) {
        LOG_FATAL("listen sockfd : %d failed.\n", sockfd_);
    }
}

int Socket::accept (inet_address *remote_addr) {
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, len);
    int cfd = ::accept4(sockfd_, (sockaddr *)&addr, &len, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (cfd >= 0) {
        remote_addr->set_sockaddr(addr);
    }
    return cfd;
}

int Socket::connect (const inet_address &server_addr) {
    return ::connect(sockfd_, (const sockaddr *)server_addr.get_sockaddr(), sizeof(sockaddr_in));
}

void Socket::close () {
    if (::close(sockfd_) < 0) {
        LOG_ERROR("close sockfd failed.\n");
    }
}

void Socket::shutdown_write()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}


void Socket::set_tcp_nodelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::set_reuse_addr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::set_reuse_port(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);
}

void Socket::set_keepalive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}