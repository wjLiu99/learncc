#include <thread>
#include <iostream>
#include <chrono>
#include <vector>
#include <numeric>
void some_function() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}
void some_other_function() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void test () {
    //t1 绑定some_function
    std::thread t1(some_function); 
    //2 转移t1管理的线程给t2，转移后t1无效
    std::thread t2 =  std::move(t1);
    //3 t1 可继续绑定其他线程,执行some_other_function
    //表达式的返回值都是右值，调用移动赋值函数
    t1 = std::thread(some_other_function);
    //4  创建一个线程变量t3
    std::thread t3;
    //5  转移t2管理的线程给t3
    t3 = std::move(t2);
    //6  转移t3管理的线程给t1
    //t1之前已经绑定了线程，重新赋值会导致之前线程调用terminate造成崩溃 **不要向已经绑定线程的变量重新赋值 
    t1 = std::move(t3);
    std::this_thread::sleep_for(std::chrono::seconds(2000));

}


//可以像uniqueptr一样，这里调用移动构造函数或者移动赋值函数
std::thread f() {
    return std::thread(some_function);
}


//封装线程管控，析构对象自动调用join和更加安全的赋值操作
class joining_thread {
    std::thread  _t;
public:
    joining_thread() noexcept = default;
    template<typename Callable, typename ...  Args>
    explicit  joining_thread(Callable&& func, Args&& ...args):
        _t(std::forward<Callable>(func),  std::forward<Args>(args)...){}
    explicit joining_thread(std::thread  t) noexcept: _t(std::move(t)){}
    joining_thread(joining_thread&& other) noexcept: _t(std::move(other._t)){}

    //该函数会去找thread的左值拷贝函数，找不到就报错
    //要移动的前提是所有成员都支持移动并且能找到移动构造函数
    //joining_thread(const joining_thread& other) noexcept: _t(std::move(other._t)){}
    joining_thread& operator=(joining_thread&& other) noexcept
    {
        //如果当前线程可汇合，则汇合等待线程完成再赋值
        if (joinable()) {
            join();
        }
        _t = std::move(other._t);
        return *this;
    }
    joining_thread& operator=(joining_thread other) noexcept
    {
        //如果当前线程可汇合，则汇合等待线程完成再赋值，这样赋值就不会有隐患
        if (joinable()) {
            join();
        }
        _t = std::move(other._t);
        return *this;
    }
    // 这样也不行 找不到const &
    //  joining_thread& operator=(const joining_thread& other) noexcept
    // {
    //     //如果当前线程可汇合，则汇合等待线程完成再赋值
    //     if (joinable()) {
    //         join();
    //     }
    //     _t = std::move(other._t);
    //     return *this;
    // }
    ~joining_thread() noexcept {
        if (joinable()) {
            join();
        }
    }
    void swap(joining_thread& other) noexcept {
        _t.swap(other._t);
    }
    std::thread::id   get_id() const noexcept {
        return _t.get_id();
    }
    bool joinable() const noexcept {
        return _t.joinable();
    }
    void join() {
        _t.join();
    }
    void detach() {
        _t.detach();
    }
    std::thread& as_thread() noexcept {
        return _t;
    }
    const std::thread& as_thread() const noexcept {
        return _t;
    }
};

void param_function (unsigned i) {

}
void use_vector() {
    std::vector<std::thread> threads;
    for (unsigned i = 0; i < 10; ++i) {
        //插入容器必须用emplaceback，因为线程没有提供拷贝构造和赋值
        //emplace调用的是thread的构造函数
        threads.emplace_back(param_function, i);
        //等价于
        //不会调用拷贝构造，直接push是会调用拷贝构造的
        auto t = std::thread(param_function, 1);
        threads.push_back(std::move(t));
    }
    for (auto& entry : threads) {
        entry.join();
    }
}




//并行计算模版
template<typename Iterator, typename T>
struct accumulate_block
{
	void operator()(Iterator first, Iterator last, T& result)
	{
		result = std::accumulate(first, last, result);
	}
};

template<typename Iterator, typename T>
T parallel_accumulate(Iterator first, Iterator last, T init)
{
    unsigned long const length = std::distance(first, last);
    if (!length)
        return init;

    unsigned long const min_per_thread = 25;    //每个线程计算的范围
    unsigned long const max_threads = (length + min_per_thread - 1) / min_per_thread; //一共需要多少个线程

    unsigned long const hardware_threads = std::thread::hardware_concurrency();     //cpu核心数

    unsigned long const num_threads = 
    std::min(hardware_threads != 0 ? hardware_threads : 2, max_threads);    //创建线程数
    unsigned long const block_size = length / num_threads;   //每个线程的时间计算区间
    std::vector<T> results(num_threads);

    std::vector<std::thread>  threads(num_threads - 1);   
        Iterator block_start = first;
    for (unsigned long i = 0; i < (num_threads - 1); ++i) //循环分配任务
    {
        Iterator block_end = block_start;
        std::advance(block_end, block_size);    
            threads[i] = std::thread(
                accumulate_block<Iterator, T>(),
                block_start, block_end, std::ref(results[i]));
        block_start = block_end;   
    }
    accumulate_block<Iterator, T>()(
        block_start, last, results[num_threads - 1]);    
        for (auto& entry : threads)
            entry.join();   
            return std::accumulate(results.begin(), results.end(), init);   
}

void use_parallel_acc() {
    std::vector <int> vec(10000);
    for (int i = 0; i < 10000; i++) {
        vec.push_back(i);
    }
    int sum = 0;
    sum = parallel_accumulate<std::vector<int>::iterator, int>(vec.begin(), 
        vec.end(), sum);
    std::cout << "sum is " << sum << std::endl;
}