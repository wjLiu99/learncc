#include "tcp_connection.h"
#include "Socket.h"
#include "channel.h"
#include "event_loop.h"
#include "logger.h"

#include <errno.h>
#include <functional>
#include <strings.h>

static event_loop *check_loop (event_loop *loop) {
    if (nullptr == loop) {
        LOG_FATAL("tcp connection loop is null");
    }
    return loop;
}

tcp_connection::tcp_connection (event_loop *loop, 
                    const std::string &name,
                    int sockfd,
                    const inet_address &local,
                    const inet_address &remote) :
                    loop_(check_loop(loop)),
                    name_(name),
                    state_(CONNECTING),
                    reading_(true),
                    socket_(new Socket(sockfd)),
                    channel_(new channel(loop, sockfd)),
                    local_(local),
                    remote_(remote),
                    high_watermark_(64 * 1024 * 1024) {

    channel_->set_read_cb(
        std::bind(&tcp_connection::handle_read, this, std::placeholders::_1)
    );
    channel_->set_write_cb(
        std::bind(&tcp_connection::handle_write, this)
    );
    channel_->set_close_cb(
        std::bind(&tcp_connection::handle_close, this)
    );
    channel_->set_error_cb(
        std::bind(&tcp_connection::handle_error, this)
    );

    LOG_INFO("tcp connection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    //心跳
    socket_->set_keepalive(true);
}

tcp_connection::~tcp_connection()
{
    LOG_INFO("tcp connection::dtor[%s] at fd=%d state=%d \n", 
        name_.c_str(), channel_->fd(), (int)state_);
}

void tcp_connection::send (const std::string &buf) {
    if (CONNECTED == state_) {
        // 如果在管理该tcp连接的eventloop线程中调用则直接发送，不在就需要通知管理该tcp连接的线程调用
        if (loop_->is_in_loop_thread()) {
            send_inloop(buf.c_str(), buf.size());
            return;
        }
        loop_->run_in_loop(
            std::bind(&tcp_connection::send_inloop, this, buf.c_str(), buf.size())
        );
    }
}

void tcp_connection::send_inloop (const void *data, size_t len) {
    ssize_t nwrote = 0;
    size_t remain = len;
    bool faulterror = false;
    if (DISCONNECTED == state_) {
        LOG_ERROR("disconnected, send failed.");
        return;
    }
    // channel处于空闲状态，且写缓冲区中没有数据可以直接发送，如果缓存区中有数据需要把待发送的数据追加到写缓冲区的后面，保证发送的有序性
    if (!channel_->is_writing() && obuf_.readable_bytes() == 0) {
        nwrote = write(channel_->fd(), data, len);
        if (nwrote >= 0) {
            remain = len - nwrote;
            // 调用send inloop是在conn的eventloop线程中，将回调函数加入线程待执行任务队列中
            // 发送完毕，执行回调
            if (remain == 0 && write_comp_cb_) {
                loop_->queue_in_loop(
                std::bind(write_comp_cb_, shared_from_this())
                );
            }
            
        } else { // nwrote < 0 发生错误
            nwrote = 0;
            if (errno != EWOULDBLOCK) {
                LOG_ERROR("tcp connection::send in loop\n");
                if (errno == EPIPE || errno == ECONNRESET) {
                    faulterror = true;
                }
            }
        }
    }
    // 未发生错误且还有数据未发送完或是写缓冲区原来还有数据待发送，写入发送缓冲区等待发送
    if (!faulterror && remain > 0) {
        size_t old = obuf_.readable_bytes();
        // 如果数据量过大，待写入的数据加缓冲区中的数据超过了高水位，调用高水位回调函数
        if (old + remain >= high_watermark_ && old < high_watermark_ && high_watermark_cb_) {
            loop_->queue_in_loop(
                std::bind(high_watermark_cb_, shared_from_this(), old + remain)
            );
        }
        // 将数据追加到写缓冲后面，监听channel的写事件
        obuf_.append((char *)data + nwrote, remain);
        if (!channel_->is_writing()) {
            channel_->enable_writing();
        }
    }
}


void tcp_connection::shutdown () {
    if (CONNECTED == state_) {
        set_state(DISCONNECTING);
        loop_->run_in_loop(
            std::bind(&tcp_connection::shutdown_inloop, this)
        );
    }
}

void tcp_connection::shutdown_inloop () {
    // 如果tcp连接正在发送，等待发送完毕会判断状态为DISCONNECTING，调用shutdown_inloop
    if (!channel_->is_writing()) {
        socket_->shutdown_write();
    }
}
void tcp_connection::force_close () {
    if (CONNECTED == state_ || DISCONNECTING == state_) {
        set_state(DISCONNECTING);
        loop_->queue_in_loop(
            std::bind(&tcp_connection::force_close_inloop, shared_from_this())
        );
    }
}

void tcp_connection::force_close_inloop()
{
  if (state_ == CONNECTED || state_ == DISCONNECTING)
  {
    handle_close();
  }
}
void tcp_connection::connect_established () {
    set_state(CONNECTED);
    channel_->tie(shared_from_this());
    channel_->enable_reading();
    conn_cb_(shared_from_this());
}

void tcp_connection::connect_destroyed () {
    // tcp server直接析构，连接状态会是CONNECTED
    if (CONNECTED == state_) {
        set_state(DISCONNECTED);
        channel_->disable_all();
        conn_cb_(shared_from_this());
    }
    // handle close调用用户设置的close cb ，状态已经是DISCONNECTED 直接调用到这里
    channel_->remove();
}

// channel触发读事件时候回调，读取sockfd中的数据到ibuf中，读取成功调用message cb，读取返回0调用关闭连接回调
void tcp_connection::handle_read (timestamp recv_time) {
    int saved_errno = 0;
    ssize_t n = ibuf_.read_fd(channel_->fd(), &saved_errno);
    if (n > 0) {
        msg_cb_(shared_from_this(), &ibuf_, recv_time);

    } else if (n == 0) {
        handle_close();
    } else {
        errno = saved_errno;
        LOG_ERROR("tcp connection handle read");
        handle_error();
    }
}

void tcp_connection::handle_write () {
    if (channel_->is_writing()) {
        int saved_errno = 0;
        ssize_t n = obuf_.write_fd(channel_->fd(), &saved_errno);
        if (n > 0) {
            obuf_.retrieve(n);
            if (obuf_.readable_bytes() == 0) {
                channel_->disable_writing();
                if (write_comp_cb_) {
                    loop_->queue_in_loop(
                        std::bind(write_comp_cb_, shared_from_this())
                    );
                }
                // 发送过程中已经调用过shutdown
                if (DISCONNECTING == state_) {
                    shutdown_inloop();
                }
            }
        } else { // n <= 0

            LOG_ERROR("TcpConnection::handleWrite");
        }
    } else { // !channel_->is_writing()

        LOG_ERROR("TcpConnection fd=%d is down, no more writing \n", channel_->fd());
    }
}
// 读取fd返回0会调用，epoll返回epollhub会调用
void tcp_connection::handle_close () {
    LOG_INFO("tcp connection handle close fd = %d, state = %d\n", channel_->fd(), (int)state_);
    set_state(DISCONNECTED);
    channel_->disable_all();

    tcp_connection_ptr conn(shared_from_this());
    conn_cb_(conn);
    close_cb_(conn);
}

void tcp_connection::handle_error () {
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
        err = errno;
    } else {
        err = optval;
    }
    LOG_ERROR("tcp connection handle error name : %s -SO_ERROR:%d\n", name_.c_str(), err);
}