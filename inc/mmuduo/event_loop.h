#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_


#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.h"
#include "timestamp.h"
#include "current_thread.h"

class channel;
class poller;

class event_loop : noncopyable {
public:
    typedef std::function<void()> functor;
    event_loop ();
    ~event_loop ();

    void loop ();
    void quit ();

    timestamp poll_return_time() const { return poll_return_time_;}

    void run_in_loop (functor cb);
    void queue_in_loop (functor cb);

    void wakeup ();

    void update_channel (channel *ch);
    void remove_channel (channel *ch);
    bool has_channel (channel *ch);

    //判断是否在创建eventloop的线程中操作该eventloop对象
    bool is_in_loop_thread () const { return thread_id_ == current_thread::tid(); }
private:
    void handle_read (); //wake up
    void do_pending_functors ();

    typedef std::vector<channel *> channel_list;

    std::atomic_bool looping_;
    std::atomic_bool quit_;

    const pid_t thread_id_;     //运行eventloop线程id
    
    timestamp poll_return_time_;    //poller返回时间
    std::unique_ptr<poller> poller_;

    int wakeup_;    //有新连接时，用于唤醒eventloop
    std::unique_ptr<channel> wakeup_channel_;

    channel_list active_channels_;
    
    std::atomic_bool calling_pending_functors_; //loop是否正在执行回调函数
    std::vector<functor> pending_functors_;     //存储回调函数
    std::mutex mtx_;


};

#endif