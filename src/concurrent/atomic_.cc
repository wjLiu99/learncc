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
template<typename T>
class threadsafe_queue
{
private:
    mutable std::mutex mut;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    threadsafe_queue()
    {}
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mut);
        data_queue.push(std::move(new_value));
        data_cond.notify_one();    //⇽-- - ①
    }
    void wait_and_pop(T& value)    //⇽-- - ②
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });
        value = std::move(data_queue.front());
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()   // ⇽-- - ③
    {
        std::unique_lock<std::mutex> lk(mut);
        data_cond.wait(lk, [this] {return !data_queue.empty(); });   // ⇽-- - ④
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }

    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return false;
        value = std::move(data_queue.front());
        data_queue.pop();
        return true;
    }

    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mut);
        if (data_queue.empty())
            return std::shared_ptr<T>();    //⇽-- - ⑤
        std::shared_ptr<T> res(
            std::make_shared<T>(std::move(data_queue.front())));
        data_queue.pop();
        return res;
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mut);
        return data_queue.empty();
    }
};

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