#include "channel.h"
#include "logger.h"
#include <sys/epoll.h>
#include "event_loop.h"

const int channel::knone_event = 0;
const int channel::kread_event = EPOLLIN | EPOLLPRI;
const int channel::kwrite_event = EPOLLOUT;

channel::channel (event_loop *loop, int fd) : loop_(loop), 
    fd_(fd), events_(0), revents_(0), idx_(-1), tied_(false) {}

channel::~channel () {}

//绑定对象，防止在执行回调函数的数据对象析构
void channel::tie (const std::shared_ptr<void> &obj) {
    tie_ = obj;
    tied_ = true;
}

//更新fd上的监听事件，需要调用eventloop传递给poller
void channel::update () {
    loop_->update_channel(this);
}
//把channel从所属的eventloop中删除 
void channel::remove () {
    loop_->remove_channel(this);
}
//poller监听到事件后处理
void channel::handle_event (timestamp recv_time) {
    if (tied_) {
        std::shared_ptr<void> exist = tie_.lock();
        if (exist) {
            handle_event_with_guard(recv_time);
        }

        return;
    }

    handle_event_with_guard(recv_time);
}

//根据返回的事件类型调用不同的回调函数
void channel::handle_event_with_guard(timestamp recv_time) {
    LOG_INFO("channel revents : %d\n", revents_);
    //断开连接,调用断开连接回调
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) {
        if (close_cb_) {
            close_cb_();
        }
    }

    if (revents_ & EPOLLERR) {
        if (error_cb_) {
            error_cb_();
        }
    }

    if (revents_ & (EPOLLIN | EPOLLPRI)) {
        if (read_cb_) {
            read_cb_(recv_time);
        }
    }

    if (revents_ & EPOLLOUT) {
        if (write_cb_) {
            write_cb_();
        }
    }
}