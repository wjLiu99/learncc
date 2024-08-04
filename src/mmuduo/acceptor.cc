#include "acceptor.h"
#include "logger.h"
#include "inet_addr.h"
#include <unistd.h>
//创建一个非阻塞的socketfd
static int create_nonblock()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

acceptor::acceptor (event_loop *loop, const inet_address &listen_addr, bool reuse) :
    loop_(loop),
    accept_socket_(create_nonblock()),
    accept_channel_(loop, accept_socket_.fd()),
    listenning_(false) {

    accept_socket_.set_reuse_addr(true);
    accept_socket_.set_reuse_port(true);
    //绑定监听地址，设置listenfd的读回调
    accept_socket_.bind_address(listen_addr);
    accept_channel_.set_read_cb(
        std::bind(&acceptor::handle_read, this)
    );
}

acceptor::~acceptor () {
    accept_channel_.disable_all();
    accept_channel_.remove();
}

void acceptor::listen () {
    listenning_ = true;
    accept_socket_.listen();
    accept_channel_.enable_reading();
}

//新连接回调函数
void acceptor::handle_read () {
    inet_address remote_addr;
    int cfd = accept_socket_.accept(&remote_addr);
    if (cfd >= 0) {
        if (new_conn_cb_) {
            //将新连接交给subloop处理
            new_conn_cb_(cfd, remote_addr);
            return;
        }
        ::close(cfd);
        return;
    }

    LOG_ERROR("%s:%s:%d accept err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    if (errno == EMFILE) {
            LOG_ERROR("%s:%s:%d sockfd reached limit! \n", __FILE__, __FUNCTION__, __LINE__);
    }

}
