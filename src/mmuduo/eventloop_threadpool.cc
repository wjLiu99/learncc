#include "eventloop_threadpool.h"
#include "eventloop_thread.h"
#include <memory>

event_loop_threadpool::event_loop_threadpool (event_loop *base, const std::string &name) :
    base_loop_(base), 
    name_(name),
    started_(false),
    num_threads_(0),
    next_(0) {

}

event_loop_threadpool::~event_loop_threadpool () {}

// 线程初始化回调函数在eventloop线程运行开始时调用
void event_loop_threadpool::start (const thread_init_cb &cb) {
    started_ = true;
    for (int i = 0; i < num_threads_; ++i) {
        char buf[name_.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);
        event_loop_thread *et = new event_loop_thread(cb, buf);
        //---
        threads_.emplace_back(et);
        loops_.push_back(et->start_loop());
    }

    if (num_threads_ == 0 && cb) {
        cb(base_loop_);
    }

}

// 只在mainloop中调用，可以保证线程安全
event_loop *event_loop_threadpool::get_nextloop () {
    event_loop *loop = base_loop_;
    if (!loops_.empty()) {
        loop = loops_[next_];
        ++next_;
        if (next_ >= loops_.size()) {
            next_ = 0;
        }
    }
    return loop;
}

std::vector<event_loop *> event_loop_threadpool::get_all_loops () {
    if (loops_.empty()) {
        return std::vector<event_loop *>(1, base_loop_);
    }

    return loops_;
}