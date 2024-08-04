#include "event_loop.h"
#include "logger.h"
#include "poller.h"
#include "channel.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>


// 防止一个线程创建多个EventLoop   thread_local
__thread event_loop *localthread_loop = nullptr;
//poller超时时间
const int poller_tmo = 10000;

int create_eventfd () {
    /*
        eventfd是为了读写而设计的，而不是用于普通文件的读写。
        eventfd的计数器是一个64位的值，写入的数据量必须足够以容纳这个计数器的新值。
        如果需要通过eventfd写入数据，必须至少写入8个字节。
        如果你尝试写入不足8个字节或超过8个字节，你会得到一个EINVAL错误
    */
    int evfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evfd < 0) {
        LOG_FATAL("eventfd create failed. errno : %d\n", errno);
    }
    return evfd;
}

event_loop::event_loop () :
        looping_(false),
        quit_(false),
        thread_id_(current_thread::tid()),
        poller_(poller::default_poller(this)),
        wakeup_(create_eventfd()),
        wakeup_channel_(new channel(this, wakeup_)),
        calling_pending_functors_(false) {


    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    //如果本线程已经创建了eventloop逻辑错误
    if (localthread_loop)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", localthread_loop, thread_id_);
    }
    else
    {
        localthread_loop = this;
    }
    //设置eventfd读事件和回调
    wakeup_channel_->set_read_cb(
        std::bind(&event_loop::handle_read, this)
    );
    wakeup_channel_->enable_reading();
}

event_loop::~event_loop () {
    wakeup_channel_->disable_all();
    wakeup_channel_->remove ();
    ::close(wakeup_);
    localthread_loop = nullptr;
}
//启动时间循环
void event_loop::loop () {
    looping_ = true;
    quit_ = false;
    while (!quit_) {
        //清空就绪事件列表
        active_channels_.clear();
        //启动事件监听，返回到就绪事件列表中
        poll_return_time_ = poller_->poll(poller_tmo, &active_channels_);
        //遍历就绪事件列表，调用channel设置好的回调函数
        for (channel *channel : active_channels_) {
            channel->handle_event(poll_return_time_);
        }
        // 处理该线程eventloop需要处理的回调函数，主要是mainloop设置的回调函数，处理新连接
        do_pending_functors();
    }

    LOG_INFO("eventloop %p stop looping.\n", this);
    looping_ = false;
}

void event_loop::quit () {
    quit_ = true;
    // 如果不是在创建eventloop的线程中调用quit需要唤醒eventloop
    if (!is_in_loop_thread()) {
        wakeup();
    }
}
//在eventloop线程中直接调用回调，不在eventloop线程中调用queueinloop通知eventloop
void event_loop::run_in_loop (functor cb) {
    if (is_in_loop_thread()) {
        cb();
        return;
    }

    queue_in_loop(cb);
}

void event_loop::queue_in_loop (functor cb) {
    {
        std::lock_guard<std::mutex> lk(mtx_);
        pending_functors_.emplace_back(cb);
    }
    // 如果不是eventloop线程需要唤醒
    // 是eventloop线程但是线程正在执行回调，执行完后又会阻塞在epollwait上，所以也需要唤醒
    if (!is_in_loop_thread() || calling_pending_functors_) {
        wakeup();
    }
}
//eventloop构造函数中给eventfd设置的读回调函数
void event_loop::handle_read () {
    uint64_t one = 1;
    ssize_t n = read(wakeup_, &one, sizeof one);
    if (n != sizeof one) {
        LOG_ERROR("eventloop::handle read error\n");
    }
}

void event_loop::wakeup () {
    uint64_t one = 1;
    ssize_t n = ::write(wakeup_, &one, sizeof one);
    if (sizeof(one) != n) {

        LOG_ERROR("eventloop wakeup failed.\n");
    }
}

void event_loop::update_channel (channel *ch) {
    poller_->update_channel(ch);
}
void event_loop::remove_channel (channel *ch) {
    poller_->remove_channel(ch);
}
bool event_loop::has_channel (channel *ch) {
    return poller_->has_channel(ch);
}
//执行回调函数
void event_loop::do_pending_functors () {
    if (0 == pending_functors_.size()){
        return;
    }
    //使用局部变量交换，减少临界区代码
    std::vector<functor> funcs_;
    //正在执行回调函数
    calling_pending_functors_ = true;
    {
        std::lock_guard<std::mutex> lk(mtx_);
        funcs_.swap(pending_functors_);
    }

    for (const functor &func : funcs_) {
        func();
    }

    calling_pending_functors_ = false;

}