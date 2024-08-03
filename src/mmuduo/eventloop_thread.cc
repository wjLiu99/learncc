#include "eventloop_thread.h"
#include "event_loop.h"
// 初始化eventloop线程，设置线程工作函数
event_loop_thread::event_loop_thread (const thread_init_cb &cb, 
        const std::string &name)
        : loop_(nullptr)
        , exit_(false)
        , thread_(std::bind(&event_loop_thread::thread_func, this), name)
        , mtx_()
        , cond_()
        , cb_(cb) {

}
event_loop_thread::~event_loop_thread () {}


// 启动工作线程，开始事件循环
event_loop *event_loop_thread::start_loop () {

        thread_.start();
        event_loop *loop = nullptr;
        {
                std::unique_lock<std::mutex> lk(mtx_);
                while (nullptr == loop_) {
                        cond_.wait(lk);
                }
                loop = loop_;
        }

        return loop;
}

//线程工作函数，创建eventloop，启动事件循环
void event_loop_thread::thread_func () {
        event_loop loop;
        if (cb_) {
                cb_(&loop);
        }

        {
                std::unique_lock<std::mutex> lk(mtx_);
                loop_ = &loop;
                cond_.notify_one();
        }

        loop.loop();
        std::lock_guard<std::mutex> lk(mtx_);
        loop_ = nullptr;
}