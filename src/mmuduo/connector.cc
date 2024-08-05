#include "connector.h"
#include "logger.h"
#include "channel.h"
#include "event_loop.h"
#include <errno.h>
#include "Socket.h"
//const int connector::max_retry_delay_ms;

static int get_socket_error (int fd) {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    int ret = 0;
    if (ret = getsockopt(fd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("tcp connection handle error name : O_ERROR:%d\n", err);
    return ret;
}

static int create_nonblock()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
    if (sockfd < 0) 
    {
        LOG_FATAL("%s:%s:%d listen socket create err:%d \n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

connector::connector (event_loop *loop, const inet_address &server_addr) : 
    loop_(loop),
    server_addr_(server_addr),
    connect_(false),
    state_(DISCONNECTED)
    //retry_delay_ms_(init_retry_delay_ms) 
{
    LOG_DEBUG("ctor connector");
}

connector::~connector () {
    LOG_DEBUG("dtor connector");
}

void connector::start () {
    connect_ = true;
    loop_->run_in_loop(
        std::bind(&connector::start_inloop, this)
    );
}

void connector::start_inloop () {
    if (!connect_) {
        LOG_DEBUG("do not connect.\n");
        return;
    }
    connect();
}

void connector::stop () {
    connect_ = false;
    loop_->queue_in_loop(
        std::bind(&connector::stop_inloop, this)
    );
}

void connector::stop_inloop () {
    if (CONNECTING == state_) {
        set_state(DISCONNECTED);
        int sockfd = remove_and_reset_channel();
        //retry(sockfd);
    }
}

// 连接成功后调用connecting
void connector::connect () {

    int sockfd = create_nonblock();
    
    int ret = ::connect(sockfd, (const sockaddr *)server_addr_.get_sockaddr(), sizeof(sockaddr_in));

    int save_errno = (ret == 0) ? 0 : errno;

    switch (save_errno)
  {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      //retry(sock.fd());
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG_ERROR("connect error in connector::startinloop errno = %d\n", save_errno);
      ::close(sockfd);
      break;

    default:

      LOG_ERROR("connect error in connector::startinloop errno = %d\n", save_errno);
        ::close(sockfd);
      // connectErrorCallback_();
      break;
  }

}

void connector::restart () {
    set_state(DISCONNECTED);
    //retry_delay_ms_ = init_retry_delay_ms;
    connect_ = true;
    start_inloop();
}

// 设置状态为connecting，已经建立好连接，将sockfd封装成一个channel，并监听读事件，一定触发读事件
void connector::connecting (int sockfd) {
    set_state(CONNECTING);
    channel_.reset(new channel(loop_, sockfd));
    channel_->set_write_cb(
        std::bind(&connector::handle_write, this)
    );
    channel_->set_error_cb(
        std::bind(&connector::handle_error, this)
    );
    //channel_->tie(shared_from_this());
    channel_->enable_writing();
}

int connector::remove_and_reset_channel () {
    channel_->disable_all();
    channel_->remove();
    int sockfd = channel_->fd();
    loop_->queue_in_loop(
        std::bind(&connector::reset_channel, this)
    );
    return sockfd;
}

void connector::reset_channel () {
    channel_.reset();
}
// 触发channel读事件后调用，此时状态为connecting，清空channel用于可用于下一个发起的tcp连接
// 设置状态为connected，将sockfd取出交给用户设置的连接回调，回调中构造tcp connection
void connector::handle_write () {
    if (CONNECTING == state_) {
        int sockfd = remove_and_reset_channel();
        //--
        set_state(CONNECTED);
        if (connect_) {
            conn_cb_(sockfd);
        }
    }
}

void connector::handle_error () {
    if (CONNECTING == state_) {
        int sockfd = remove_and_reset_channel();
        get_socket_error(sockfd);
    }
}
