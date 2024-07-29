 #include <atomic>
 #include <thread>
 #include <iostream>
 #include <assert.h>

/*
宽松内存序
1 作用于原子变量
2 不具有synchronizes-with关系
3 对于同一个原子变量，在同一个线程中具有happens-before关系, 在同一线程中不同的原子变量不具有happens-before关系，可以乱序执行。
4 多线程情况下不具有happens-before关系
*/
std::atomic<bool> x, y;
std::atomic<int> z;
void write_x_then_y() {
    x.store(true, std::memory_order_relaxed);  // 
    y.store(true, std::memory_order_relaxed);  // y很有可能先执行
}

void read_y_then_x() {
    while (!y.load(std::memory_order_relaxed)) { // y先执行的情况这边循环退出x还没有变为true，z不会执行++操作导致断言失败
        std::cout << "y load false" << std::endl;
    }
    if (x.load(std::memory_order_relaxed)) { //
        ++z;
    }
}


void TestOrderRelaxed() {
    std::thread t1(write_x_then_y);
    std::thread t2(read_y_then_x);
    t1.join();
    t2.join();
    assert(z.load() != 0); // 
}

/*
    先行，a先行与b，表示如果a先于b执行，a的结果对于b是可见的  只是在c++语义层面的规定，是在c编译器编排指令的顺序
    同步，先行，依赖。。。
    三种模型 全局一致性模型，同步模型，宽松模型
    全局一致性，写的时候会触发写失效，别的线程不能写，也不能读，写完要同步到所有cpu核心的缓存中
    同步模型：
    宽松：保证操作的原子性和单线程修改顺序的一致性，但是无法保证能被其他cpu看到，在一个线程内修改变量的顺序在另个线程看来
    可能是不一致的，后改的可能已经被另个线程读出来了，先修改的读出来的可能是旧值
    从计算机指令层面理解，x++和y++变量的修改没有依赖关系，很有可能y++的指令先执行，
    在单线程内也有可能发生，在c++语义层面就理解为x++先于y++执行

    锁的力度是最大的，加锁解锁之间的代码全局看到都是一致的
    weak读到的可能是旧值
    无锁环形队列不太适合智能指针，pop之后指针还残留在队列中导致资源可能不能及时释放
    无锁队列使用自定义对象时，重试会重复的调用构造函数或者拷贝构造，开销比较大不适合
*/


//栅栏机制，release之前的操作不会被编排到release之后，acquire之后的操作不会被编排到acquire之前
//只要保证release和acquire栅栏之间的代码的同步关系就能保证1先行与6
void write_x_then_y_fence()
{
    x.store(true, std::memory_order_relaxed);  //1
    std::atomic_thread_fence(std::memory_order_release);  //2
    y.store(true, std::memory_order_relaxed);  //3
}
void read_y_then_x_fence()
{
    while (!y.load(std::memory_order_relaxed));  //4 
    std::atomic_thread_fence(std::memory_order_acquire); //5
    if (x.load(std::memory_order_relaxed))  //6
        ++z;
}

#include <mutex>
#include <queue>
#include <condition_variable>
// template<typename T>
// class threadsafe_queue
// {
// private:
//     mutable std::mutex mut;
//     std::queue<T> data_queue;
//     std::condition_variable data_cond;
// public:
//     threadsafe_queue()
//     {}
//     void push(T new_value)
//     {
//         std::lock_guard<std::mutex> lk(mut);
//         data_queue.push(std::move(new_value));
//         data_cond.notify_one();    //⇽-- - ①
//     }
//     void wait_and_pop(T& value)    //⇽-- - ②
//     {
//         std::unique_lock<std::mutex> lk(mut);
//         data_cond.wait(lk, [this] {return !data_queue.empty(); });
//         value = std::move(data_queue.front());
//         data_queue.pop();
//     }

//     std::shared_ptr<T> wait_and_pop()   // ⇽-- - ③
//     {
//         std::unique_lock<std::mutex> lk(mut);
//         data_cond.wait(lk, [this] {return !data_queue.empty(); });   // ⇽-- - ④
//         std::shared_ptr<T> res(
//             std::make_shared<T>(std::move(data_queue.front())));
//         data_queue.pop();
//         return res;
//     }

//     bool try_pop(T& value)
//     {
//         std::lock_guard<std::mutex> lk(mut);
//         if (data_queue.empty())
//             return false;
//         value = std::move(data_queue.front());
//         data_queue.pop();
//         return true;
//     }

//     std::shared_ptr<T> try_pop()
//     {
//         std::lock_guard<std::mutex> lk(mut);
//         if (data_queue.empty())
//             return std::shared_ptr<T>();    //⇽-- - ⑤
//         std::shared_ptr<T> res(
//             std::make_shared<T>(std::move(data_queue.front())));
//         data_queue.pop();
//         return res;
//     }
    
//     bool empty() const
//     {
//         std::lock_guard<std::mutex> lk(mut);
//         return data_queue.empty();
//     }
// };

// int main () {

//     threadsafe_queue<int> q;
//     q.push(5);
//     int a = 0;
//     q.wait_and_pop(a);
//     std::cout << "a = " << a << std::endl;

// }

/*
std:atomic_flag实现自旋锁
*/

class spinlock {
public:
    void lock () {
        while (flag_.test_and_set(std::memory_order_acquire));
    }

    void unlock () {
        flag_.clear(std::memory_order_release);
    }
private:
    std::atomic_flag flag_ = ATOMIC_FLAG_INIT;
};

/*
内存序是指的是指令在多核处理器或多处理器系统中执行时，对内存操作（读写内存）进行排序和控制的机制。
在多核处理器中，不同的核心（或处理器）可能会并行执行指令，这就引入了需要协调和同步各核心访问共享内存区域的需求。

内存序描述了对存储系统中的读写操作的顺序限制。在现代处理器中，由于性能优化和指令乱序执行的特性，处理器在执行指令时并不总是按照代码中编写的顺序依次执行。
这意味着，如果没有适当的同步和控制机制，处理器可能会以不同的顺序执行内存访问，这可能会导致一些程序错误或者不符合预期的行为。

为了解决这个问题，处理器通过硬件机制（如内存屏障、缓存一致性协议等）来确保对共享内存的读写操作在多核处理器中保持一致的顺序。
这些机制确保了程序员在编写并发程序时可以依赖于某些保证，例如在一个线程写入数据之后，另一个线程可以看到更新后的数据值。

总结起来，内存序是处理器为了确保多核处理器中并发执行的正确性而实施的一种顺序管理机制，它涉及到处理器如何处理和排序对共享内存的读写操作，以确保程序执行的正确性和一致性。
*/

/*
memory_order_seq_cst表示"顺序一致性"（sequentially consistent）。
顺序一致性是一种最强的内存模型，保证在多线程并发环境下，所有的内存操作都会按照程序中指定的顺序来执行，
所有的原子操作都遵循全局的全序（total order），即一个线程的所有操作按照程序顺序执行，且在所有线程中看起来都是按照相同的顺序执行的。
所有的内存操作都表现出顺序一致性，即在任意时刻，都存在一个全局的时间线，所有的线程都能够看到一致的操作顺序。
*/

/*

memory_order_release 是一个内存顺序选项，用于原子操作和多线程同步的语境中。它指定了一种内存访问的顺序约束，主要用于实现同步的释放语义。

当一个原子操作使用 memory_order_release 时，它确保：

所有在该原子操作之前的内存写操作（包括非原子的写操作）在该原子操作之前完成。
这意味着，如果一个线程通过释放语义的操作（例如 store with memory_order_release）将数据存储到共享内存中，
那么这些存储操作必须在释放语义之前的所有操作（包括之前的读写操作）完成之后对其他线程可见。
在 C++ 中，memory_order_release 是一种较轻量级的同步机制，通常与 memory_order_acquire 或 memory_order_acq_rel 一起使用，
以实现更复杂的同步需求，例如双重检查锁模式（double-checked locking pattern）或无锁数据结构的实现。

memory_order_release 提供了一种机制，确保在释放操作之后的一些操作不会被提前到释放操作之前发生，从而帮助维护线程之间的正确同步和可见性。
*/

/*
在多线程的环境下，并不能保证多线程指令运行的先后顺序，只能保证编译器在重排指令的顺序，如果一个线程的指令a先于另一个线程的指令b运行，那么a的结果可以被b看到，这就是同步关系
有可能看不到是因为cpu对内存的操作顺序是不同的，cpu操作完成不一定立刻把数据写入内存，可能写入自身的缓存择机写入内存，读取时也可能不从内存中读取而从缓存中读取
所以多线程的操作要同步就需要现在操作内存的顺序。
*/

/*
    全局一致性是保证只要某个线程对原子变量进行了修改，所有的线程都能读到最新的值
    宽松内存序值能保证在一个线程内对同一原子变量操作的顺序先行关系，对不同变量以及不同线程内同一变量没有同步关系

*/


//用原子变量解决单例双检查锁的reorder问题
class singleton_atomic {

public:
    std::shared_ptr<singleton_atomic> &get_instance () {
        if (flag_.load(std::memory_order_acquire)) {
            return instance_;
        }
        
        mtx_.lock();
        if (flag_.load(std::memory_order_relaxed)) {
            return instance_;
            mtx_.unlock();
        }

        instance_ = std::shared_ptr<singleton_atomic>(new singleton_atomic);
        flag_.store(true, std::memory_order_release);   //读到flag为true的时候，保证之前的写操作已经完成
        mtx_.unlock();
        return instance_;
        
    }
protected:
    singleton_atomic(){}
    singleton_atomic (const singleton_atomic&) = delete;
    singleton_atomic &operator=(const singleton_atomic &) = delete;

private:
    static std::shared_ptr<singleton_atomic> instance_;
    static std::mutex mtx_;
    static std::atomic<bool> flag_;

};

std::shared_ptr<singleton_atomic> singleton_atomic::instance_ = nullptr;
std::mutex singleton_atomic::mtx_;
std::atomic<bool> singleton_atomic::flag_ = false;



class MyObject {
public:
    std::string name;

public:
    // Constructor
    MyObject(const std::string& n) : name(n) {
        std::cout << "MyObject(const std::string& n) '" << name << "'\n";
    }
    MyObject(const std::string& n , int a) : name(n) {
        std::cout << "MyObject(const std::string& n , int a) '" << name << "'\n";
    }

    // Destructor
    ~MyObject() {
        std::cout << "~MyObject()'" << name << "'\n";
    }

    // Copy constructor (left value)
    MyObject(const MyObject& other) : name(other.name) {
        std::cout << "MyObject(const MyObject& other) '" << other.name << "' (copy constructor)\n";
    }

    // Move constructor (right value)
    MyObject(MyObject&& other) noexcept : name(std::move(other.name)) {
        std::cout << "MyObject(MyObject&& other) '" << name << "' (move constructor)\n";
    }

    // Copy assignment operator (left value)
    MyObject& operator=(const MyObject& other) {
        if (this != &other) {
            name = other.name;
            std::cout << "MyObject& operator=(const MyObject& other) '" << other.name << "' (copy assignment)\n";
        }
        return *this;
    }

    // Move assignment operator (right value)
    MyObject& operator=(MyObject&& other) noexcept {
        if (this != &other) {
            name = std::move(other.name);
            std::cout << "MyObject& operator=(MyObject&& other) '" << name << "' (move assignment)\n";
        }
        return *this;
    }
};


//用锁实现并发环形队列
template<typename T>
class ring_queue : private std::allocator<T> {
public:
    ring_queue (size_t size) : max_(size +1), data_(std::allocator<T>::allocate(max_)), head_(0), tail_(0) {}
    ring_queue (const ring_queue&) = delete;
    ring_queue &operator=(const ring_queue &) = delete;
    ~ring_queue () {
        std::lock_guard<std::mutex> lk(mtx_);
        while (head_ != tail_) {
            //调用对象析构函数
            std::allocator<T>::destroy(data_ + head_);
            head_ = (head_ + 1) % max_;
        }
        //回收内存
        std::allocator<T>::deallocate(data_, max_);
    }

    template<typename ...Args>
    bool emplace (Args && ...arg) {
        std::lock_guard<std::mutex> lk(mtx_);
        if ((tail_ + 1) % max_ == head_) {
            std::cout << "queue full" << std::endl;
            return false;
        }

        std::allocator<T>::construct(data_ + tail_, std::forward<Args>(arg)...);
        tail_ = (tail_ + 1) % max_;
        return true;
    }

    template<typename ...Arg>
    bool push (Arg && ...arg) {
        return emplace(std::forward<Arg>(arg)...);
    }

    bool pop (T &val) {
        std::lock_guard<std::mutex> lk(mtx_);
        if (head_ == tail_) {
            std::cout << "queue empty" << std::endl;
            return false;
        }

        val = std::move(data_[head_]);
        //不析构会可能会造成内存泄漏，如果全部存储对象的全部成员都支持移动就不用析构。或者只存储智能指针，用户传进来的数据用makeshared拷贝一份生成智能指针保存在队列
        std::allocator<T>::destroy(data_ + head_);
        head_  = (head_ + 1) & max_;
        return true;

    }


private:
    size_t max_;
    T* data_;
    size_t head_;
    size_t tail_;
    std::mutex mtx_;
    


};
/*
    问题：入队出队用的同一把锁，增加了耦合度，入队出队只能串行
*/
void ring_queue_test () {
    ring_queue<MyObject> q(2);
    MyObject o1("hi");
    
    q.push(o1);
    q.push(o1);
    q.push(o1);
    q.pop(o1);
    q.push(o1);

}

//无锁队列 unlock
template<typename T>
class ulring_queue : private std::allocator<T> {
public:
    ulring_queue (size_t size) : max_(size +1), data_(std::allocator<T>::allocate(max_)), head_(0), tail_(0), using_(false) {}
    ulring_queue (const ulring_queue&) = delete;
    ulring_queue &operator=(const ulring_queue &) = delete;
    ~ulring_queue () {
        //希望未使用，修改为已使用,不断重试
        bool use_expected = false;
        bool use_desired = true;
        do {
            use_expected = false;
            use_desired = true;
        } while (!using_.compare_exchange_strong(use_expected, use_desired));
        while (head_ != tail_) {
            //调用对象析构函数
            std::allocator<T>::destroy(data_ + head_);
            head_ = (head_ + 1) % max_;
        }
        //回收内存
        std::allocator<T>::deallocate(data_, max_);

        do {
            use_expected = true;
            use_desired = false;          
        } while (!using_.compare_exchange_strong(use_expected, use_desired));
    }

    template<typename ...Args>
    bool emplace (Args && ...arg) {
        bool use_expected = false;
        bool use_desired = true;
        do {
            use_expected = false;
            use_desired = true;
        } while (!using_.compare_exchange_strong(use_expected, use_desired));

        if ((tail_ + 1) % max_ == head_) {
            std::cout << "queue full" << std::endl;
            do {
            use_expected = true;
            use_desired = false;          
        } while (!using_.compare_exchange_strong(use_expected, use_desired));

            return false;
        }

        std::allocator<T>::construct(data_ + tail_, std::forward<Args>(arg)...);
        tail_ = (tail_ + 1) % max_;

        do {
            use_expected = true;
            use_desired = false;          
        } while (!using_.compare_exchange_strong(use_expected, use_desired));
        return true;
    }

    template<typename ...Arg>
    bool push (Arg && ...arg) {
        return emplace(std::forward<Arg>(arg)...);
    }

    bool pop (T &val) {
            bool use_expected = false;
            bool use_desired = true;
        do {
            use_expected = false;
            use_desired = true;
        } while (!using_.compare_exchange_strong(use_expected, use_desired));
        if (head_ == tail_) {
            std::cout << "queue empty" << std::endl;
            do {
            use_expected = true;
            use_desired = false;          
            } while (!using_.compare_exchange_strong(use_expected, use_desired));
            return false;
        }

        val = std::move(data_[head_]);
        //不析构会可能会造成内存泄漏，如果全部存储对象的全部成员都支持移动就不用析构。或者只存储智能指针，用户传进来的数据用makeshared拷贝一份生成智能指针保存在队列
        std::allocator<T>::destroy(data_ + head_);
        head_  = (head_ + 1) & max_;
        do {
            use_expected = true;
            use_desired = false;          
        } while (!using_.compare_exchange_strong(use_expected, use_desired));
        return true;

    }


private:
    size_t max_;
    T* data_;
    size_t head_;
    size_t tail_;
    std::atomic<bool> using_;
    
};
/*
这样的无锁队列push和pop的耦合度太高，同一时刻只有一个线程pop或者push
应该考虑解耦，
*/
void ulring_queue_test () {
    ulring_queue<MyObject> q(2);
    MyObject o1("hi");
    
    q.push(o1);
    q.push(o1);
    q.push(o1);
    q.pop(o1);
    q.push(o1);

}

//将头指针和尾指针设置成原子变量解耦push和pop操作，通过不断的重试比较交换，保证只有一个线程能修改成功
template<typename T>
class ulring_queue1 : private std::allocator<T> {
public:
    ulring_queue1 (size_t size) : max_(size +1), data_(std::allocator<T>::allocate(max_)), head_(0), tail_(0), tail_update_(0) {}
    ulring_queue1 (const ulring_queue1&) = delete;
    ulring_queue1 &operator=(const ulring_queue1 &) = delete;
    ~ulring_queue1 () {
       
    }


    // template<typename ...Arg>
    // bool push (Arg && ...arg) {
        
    // }
    
    bool push1 (T &val) {
        size_t t;
        do {
            t = tail_.load();
            if ((t + 1) % max_ == head_.load()) {
                std::cout << "queue full" << std::endl;
                return false;
            }
            //存在问题，多线程修改tail的时候修改成功了，但是值被别的线程覆盖了有可能被覆盖了
            data_[tail_] = val;
        } while (!tail_.compare_exchange_strong(t, (t + 1) % max_));  
        return true; 
    }

    bool push2 (T &val) {
        size_t t;
        do {
            t = tail_.load();
            if ((t + 1) % max_ == head_.load()) {
                std::cout << "queue full" << std::endl;
                return false;
            }
            
            
        } while (!tail_.compare_exchange_strong(t, (t + 1) % max_));  
        //拿出来还是有问题，如果另一个线程pop的时候，tail已经修改完毕，但是值还没有更新，就pop到了旧值，解决办法增加一个尾部更新的原子变量记录更新完毕的位置
        data_[tail_] = val;
        return true;       
    }

    bool push3 (T &val) {
        size_t t;
        do {
            //可采用宽松内存序，应为下面可以不断重试，重试时读改写
            t = tail_.load(std::memory_order_relaxed);
            if ((t + 1) % max_ == head_.load(std::memory_order_acquire)) {
                std::cout << "queue full" << std::endl;
                return false;
            }
            
            //比较成功使用release内存序修改，比较失败用relaxed重试
        } while (!tail_.compare_exchange_strong(t, (t + 1) % max_, std::memory_order_release, std::memory_order_relaxed));  
        data_[t] = val;
        size_t u;
        do {
            u = t;
        }
        while (!tail_update_.compare_exchange_strong(u, (u + 1) % max_, std::memory_order_release, std::memory_order_relaxed));
        
        return true;       
    }

    bool pop (T &val) {
        size_t h;
        do {
            h = head_.load(std::memory_order_relaxed);
            if (h == tail_.load(std::memory_order_acquire)) {
                std::cout << "queue empty" << std::endl;
                return false;
            }
            //说明数据还没有更新完,与push对tailupdate的操作需要同步
            if (h == tail_update_.load(std::memory_order_acquire)) {
                std::cout << "data updating" << std::endl;
                return false;
            }
            //不可用使用move，多线程如果同时都走到这里，只有一个线程能够move成功，其他的线程得到的都是无效的值
            //如果一个得到无效值的线程修改head成功，就造成数据丢失了，所以采用赋值操作不改变队列中数据
            val = data_[head_];
        } while (!head_.compare_exchange_strong(h, (h + 1) % max_, std::memory_order_release, std::memory_order_relaxed));
        return true;
        
    }


private:
    size_t max_;
    T* data_;
    std::atomic<size_t> head_;
    std::atomic<size_t> tail_;
    std::atomic<size_t> tail_update_;
    
    
};

void ulring_queue_test1 () {
    ulring_queue1<MyObject> q(2);
    MyObject o1("hi");
    
    q.push3(o1);

    q.push3(o1);
    q.push3(o1);
    q.pop(o1);
    q.pop(o1);
    q.push3(o1);

}

void ex_test () {
    std::atomic<int> a = 5;
    int b = 0;
    a.compare_exchange_strong(b, 10);
    std::cout << b << std::endl;
}

/*
无锁编程
优势
无锁高并发. 虽然存在循环重试, 但是这只会在相同操作并发的时候出现. push 不会因为与 pop 并发而重试, 反之亦然.

缺陷
这样队列只应该存储标量, 如果存储类对象时，多个push线程只有一个线程push成功，而拷贝复制的开销很大，其他线程会循环重试，每次重试都会有开销。
建议存储对象的智能指针
*/
/*
release和acquire模型只能保证两个线程之间的同步，不能保证全局一致也就是所有线程看到的执行顺序一致
*/

/*
如果操作 A “先行于” 操作 B，那么可以保证在多线程环境下，操作 A 的效果一定在操作 B 之前可见。
程序顺序规则（Program Order Rule）：
在单个线程内，程序中的操作按照顺序执行。
如果操作 A 在代码中出现在操作 B 之前，则操作 A 先行于操作 B
意思是如果a操作在b操作之前执行，b可以读到a的结果，是如果a比b先执行，但是实际上不一定是a比b先执行，这是因为编译器对指令优化的结果，所以先行只是一种语义层面的概念
可以添加内存序来限制编译器对指令重排的结果，保证a先于b执行，这样就能保证a先行与b

同步关系
如果一个操作 A 在一个线程中与一个操作 B 在另一个线程中有同步关系，且操作 A 发生在操作 B 之前，那么操作 A 先行于操作 B。

依赖关系
如果操作 A 依赖于操作 B 的结果（即 A 读取了 B 的写入结果），那么操作 B 先行于操作 A。

*/
#include "ts_queue.h"
void queue_test () {
    threadsafe_queue_ptr<MyObject> q;
    // MyObject o1("hi");
    q.push("hello", 5);
}

void threadsafe_queue_l_test () {
    threadsafe_queue_l<MyObject> q;
    MyObject o1("hello");
    // MyObject o2("hi");
    q.push(o1);
    q.push(o1);
    q.push(o1);

    q.push(o1);
    q.emplace("llll", 5);
    MyObject o2("hi");
    // q.try_pop_head(o2);
    q.wait_pop_head(o2);

    // std::thread t1([&](){
    //     // std::this_thread::sleep_for(std::chrono::seconds(5));
    //     int i = 0;
    //     while (i++ < 1000) {
    //         q.push(o1);
    //     }
        
    // });
    // std::thread t2([&](){
    //     // std::this_thread::sleep_for(std::chrono::seconds(5));
    //     int i = 0;
    //     while (i++ < 1000) {
    //         q.push(o1);
    //     }
    // });
    // int i = 0;
    // while (i<2000) {
    //     std::shared_ptr<MyObject> o2 = q.wait_pop_head();
    //     std::cout <<"========" << o2->name << std::endl;
    //     i++;
    // }
    
    std::cout << "pop done" << std::endl;
    // t1.join();
    // t2.join();

    

}
#include "ts_hash_tbl.h"
void hash_test () {
    ts_hash_tbl<int, MyObject> h;
    // MyObject o1("hello");
    h.insert(1, MyObject("hi"));
    // h.remove(1);
    h.insert(2, MyObject("hello"));
    // MyObject o2 =  h.find(1, MyObject(""));
}