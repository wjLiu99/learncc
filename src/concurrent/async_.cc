#include <iostream>
#include <future>
#include <chrono>
#include <string>
#include "thrdp.h"

// 模拟一个异步任务，比如从数据库中获取数据
std::string fetchDataFromDB(std::string query) {
	std::this_thread::sleep_for(std::chrono::seconds(5));
	return "Data: " + query;
}

void async_test() {
	// 使用 std::async 异步调用 
    // std::launch::deferred 延迟调用，在std::future::get()或std::future::wait()时才调用
    // 默认std::launch::async | std::launch::deferred，操作系统择机调用，可能会判断当前cpu状态之类的
	std::future<std::string> res = std::async(std::launch::async, fetchDataFromDB, "Data");

	// 在主线程中做其他事情
	std::cout << "Doing something else..." << std::endl;

	// 从 future 对象中获取数据，会阻塞等待返回结果，但只能获取一次 wait可以调用多次，但是不会获取返回值
	std::string dbData = res.get();
	std::cout << dbData << std::endl;

}

/*
    std::packaged_task 是一个可调用对象，封装了一个任务，可以在另个线程运行通过future获取返回值或者异常
    task只支持右值拷贝，所有传参时要使用移动语义，移动之后外部就不能使用task对象了
*/

/*
    promise优势：不用等到任务执行完就能获得返回结果，通过setvalue，外部get就会返回结果，也是只能移动构造
    可以抛出异常，外部可以get捕获该异常  在子线程 抛出的异常在主线程一定要捕获，不然主线程崩溃都找不到原因
    类似go语言中的channel
*/

void set_value(std::promise<int> prom) {
    // 设置 promise 的值
    prom.set_value(10);
}
int promise_test() {
    // 创建一个 promise 对象
    std::promise<int> prom;
    // 获取与 promise 相关联的 future 对象
    std::future<int> fut = prom.get_future();
    // 在新线程中设置 promise 的值
    std::thread t(set_value, std::move(prom));
    // 在主线程中获取 future 的值
    std::cout << "Waiting for the thread to set the value...\n"; 
    // 调用get的时候一定要保证future变量生存
    std::cout << "Value set by the thread: " << fut.get() << '\n';
    t.join();
    return 0;
}

void thrpool_test () {
    ThreadPool pool;
    pool.start();
    int m = 0;
    //std::ref实现了仿函数，把m封装到reference_wrapper里，实际是封装了m的地址，重载了括号运算符
    //通过线程池要修改局部变量的值要么捕获局部变量的引用，要么就用ref包装传递参数
    pool.submitTask([](int &m) {
        m = 1024;
        std::cout << "inner set m = " << m << std::endl;
        std::cout << "m add is " << &m <<std::endl;
    }, std::ref(m));

    //这样不会修改m的值，都是完美转发怎么修改不了m的值呢
    //问题出在bind上，bind在构造的时候会使用Mypair第二个参数会使用decay将参数打包成一个元组，这时候就会去掉引用生成一个副本
    //使用ref最后也是一个副本，传递给回调函数也是右值，回调函数接收一个int型的参数，wrapper能转int吗
    //ref提供了隐式转换就是重载的括号运算符
    pool.submitTask([](int &m) {
        m = 1024;
        std::cout << "inner set m = " << m << std::endl;
        std::cout << "m add is " << &m <<std::endl;
    }, m); 
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "outer set m = " << m << std::endl;
    std::cout << "m add is " << &m <<std::endl;
}

/*
async  缺点：一次性线程，频繁调用就会创建和销毁
    优点：不用管理线程
    一般使用线程加条件变量或者线程池 
函数式编程，不关注容器也不关注类型  
async会择机启动线程 
线程池单例，构造私有  future是不允许拷贝的但是可以返回局部变量给外部使用右值
actor模式只需要维护一个线程安全的队列


go 面向接口编程 interface  channel 协程  函数
*/

//使用async造成死锁
//主要原因是future析构要等待任务执行结束，不然会阻塞等待
//普通想法：async完全异步，主线程加锁之后调用async，函数内部也要加锁，虽然加不上但是等主线程出局部作用域时释放锁就可以加上
//但是future出局部作用域要析构会等待任务执行完毕，这样就造成了循环等待,即使future定义在外面也一样会死锁
//捕获局部变量的引用非常危险
//解决办法，1 函数接收future引用的参数由外部传递进来在加上局部作用域让mutex能解锁，解决锁失效只能调用future的wait等了，还是一个阻塞调用
void DeadLock() {
    std::mutex  mtx;
    std::cout << "DeadLock begin " << std::endl;
    std::lock_guard<std::mutex>  dklock(mtx);
    {
            
    
        std::future<void> futures = std::async(std::launch::async, [&mtx]() {
        std::cout << "std::async called " << std::endl;
        std::lock_guard<std::mutex>  dklock(mtx);
        std::cout << "async working...." << std::endl;
        });

    }
 
    
    std::cout << "DeadLock end " << std::endl;
}

//封装纯异步操作，future只有在引用计数为0的时候才析构
template<typename Func, typename... Args  >
auto  ParallenExe(Func&& func, Args && ... args) -> std::future<decltype(func(args...))> {
    typedef    decltype(func(args...))  RetType;
    std::function<RetType()>  bind_func = std::bind(std::forward<Func>(func), std::forward<Args>(args)...);
    std::packaged_task<RetType()> task(bind_func);
    //future计数+1，此时future和task绑定引用计数为2
    auto rt_future = task.get_future();
    std::thread t(std::move(task));
    t.detach();
    //外部有变量接收引用计数不变，没有接收引用计数-1，但是不为0不会析构
    return rt_future;
}

int asyncFunc() {
    std::this_thread::sleep_for(std::chrono::seconds(3));
    std::cout << "this is asyncFunc" << std::endl;
    return 0;
}
void func1(std::future<int>& future_ref) {
    std::cout << "this is func1" << std::endl;
    future_ref = std::async(std::launch::async, asyncFunc);
}
void func2(std::future<int>& future_ref) {
    std::cout << "this is func2" << std::endl;
    auto future_res = future_ref.get();
    if (future_res == 0) {
        std::cout << "get asyncFunc result success !" << std::endl;
    }
    else {
        std::cout << "get asyncFunc result failed !" << std::endl;
        return;
    }
}
//提供多种思路，这是第一种
void first_method() {
    std::future<int> future_tmp;
    func1(future_tmp);
    func2(future_tmp);
}
// int main () {
    
//     std::cout << "main" << std::endl;
    
// }