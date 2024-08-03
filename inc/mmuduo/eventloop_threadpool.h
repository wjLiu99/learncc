#ifndef _EVENTLOOP_THREADPOOL_H_
#define _EVENTLOOP_THREADPOOL_H_

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class event_loop;
class event_loop_thread;

class event_loop_threadpool : noncopyable {
public:
    typedef std::function<void(event_loop *)> thread_init_cb;
    event_loop_threadpool (event_loop *base_loop, const std::string &name);
    ~event_loop_threadpool ();

    void set_threadnum (int num) { num_threads_ = num; }
    void start (const thread_init_cb &cb = thread_init_cb());
    //多线程下轮询返回subloop
    event_loop *get_nextloop ();

    std::vector<event_loop *> get_all_loops ();

    bool started () const { return started_; }
    const std::string name () const { return name_; }


private:

    event_loop *base_loop_;
    std::string name_;
    bool started_;
    int num_threads_;
    int next_;
    std::vector<std::unique_ptr<event_loop_thread>> threads_;
    std::vector<event_loop *> loops_;


};
#endif