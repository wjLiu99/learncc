#ifndef _THREAD_H_
#define _THREAD_H_
#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable {

public:
    typedef std::function<void()> thread_func;

    explicit Thread (thread_func func, const std::string &name = std::string());
    ~Thread ();

    void start ();
    void join ();

    bool started () const { return started_; }
    pid_t tid () const { return tid_; }
    const std::string &name () const { return name_; }
    
    static int num_created () { return num_created_; }

private:
    void set_default_name ();

    bool started_;
    bool joined_;
    std::shared_ptr<std::thread> thread_;

    pid_t tid_;
    thread_func func_;
    std::string name_;
    static std::atomic_int num_created_;
};
#endif