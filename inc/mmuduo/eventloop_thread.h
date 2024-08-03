#ifndef _EVENTLOOP_THREAD_H_
#define _EVENTLOOP_THREAD_H_

#include "noncopyable.h"
#include "Thread.h"

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

class event_loop;

//封装eventloop工作线程
class event_loop_thread : noncopyable {
public:

    typedef std::function<void(event_loop *)> thread_init_cb;
    event_loop_thread (const thread_init_cb &cb = thread_init_cb(), const std::string &name = std::string());
    ~event_loop_thread ();

    event_loop *start_loop ();

private:

    void thread_func ();
    
    event_loop *loop_;
    bool exit_;
    Thread thread_;
    std::mutex mtx_;
    std::condition_variable cond_;
    thread_init_cb cb_;
};


#endif