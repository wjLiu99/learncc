#include "Thread.h"
#include "current_thread.h"
#include <semaphore.h>
using namespace std;

std::atomic_int Thread::num_created_(0);

Thread::Thread (thread_func func, const string &name):
    started_(false),
    joined_(false),
    tid_(0),
    func_(move(func)),
    name_(name) {

    set_default_name();
}

Thread::~Thread () {
    if (started_ && !joined_) {
        thread_->detach();
    }
}

void Thread::start () {
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        tid_ = current_thread::tid();
        sem_post(&sem);
        func_();
    }));
    //等待线程设置tid
    sem_wait(&sem);
}

void Thread::join () {
    joined_ = true;
    if (thread_->joinable()) {
         thread_->join();
    }

}
// 设置默认名字
void Thread::set_default_name () {
    int num = ++num_created_;
    if (name_.empty()) {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "thread%d", num);
        name_ = buf;
    }
}