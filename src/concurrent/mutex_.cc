#include <iostream>
#include <mutex>
#include <thread>
#include <chrono>
#include <stack>
#include <climits>
#include <queue>

#include <condition_variable>
std::mutex mutex;
int shared = 100;

void mlock() {
    while (1) {
        //RAII资源获取时初始化，析构时释放资源
        //降低锁的粒度加上局部作用域
        {
            std::lock_guard<std::mutex> lg(mutex);
            // mutex.lock();
            shared++;
            std::cout << "cur thread is " << std::this_thread::get_id() << std::endl;
            std::cout << "shared data is " << shared << std::endl;

        }
        
        // mutex.unlock();
        //不睡眠会一直抢cpu时间片
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

void lock_test () {
    std::thread t1(mlock);
    std::thread t2([](){
        while (1) {
            mutex.lock();
            shared--;
            std::cout << "cur thread is " << std::this_thread::get_id() << std::endl;
            std::cout << "shared data is " << shared << std::endl;
            mutex.unlock();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    });
    t1.join();
    t2.join();
}


struct empty_stack : std::exception
{
    const char* what() const throw();
};


template <typename T>
class thrsafe_stack {
private:
    std::stack<T> data_;
    //可变的，如果函数是const修饰的表示不修改成员变量的值，但是有一些变量需要修改如加解锁操作，所有要声明为可变的
    //如判断栈是否为空需要加解锁。
    mutable std::mutex m_;  

public:
    thrsafe_stack() {}
    thrsafe_stack (const thrsafe_stack& other) {
        std::lock_guard<std::mutex> lock(other.m_);
        data_ = other.data_;

    }

    thrsafe_stack& operator= (const thrsafe_stack&) = delete;


    T pop()
    {
        std::lock_guard<std::mutex> lg(m_);
        auto element = data_.top();
        data_.pop();
        //pop出来之后要返回给外部，但是如果外部没有足够的内存进行拷贝就会抛出异常，怎么优雅的退出，将已经pop出来的数据落盘
        //可以使用智能指针,如pop1，主要是减少内存拷贝的开销，缩减成一次拷贝构造，返回智能指针拷贝消耗比较小
        return element;
    }

    std::shared_ptr<T> pop1()
    {
        std::lock_guard<std::mutex> lg(m_);
        if (data_.empty()) {
            throw empty_stack();
        }
        /*top返回的是引用类型*/
        std::shared_ptr<T> const res(std::make_shared<T>(data_.top()));
        data_.pop();
        
        return res;
    }
    //外部已经开辟好空间传进来，也是减少拷贝
    void pop2(T& value)
    {
        std::lock_guard<std::mutex> lock(m_);
        if (data_.empty()) throw empty_stack();
        value = data_.top();
        data_.pop();
    }

   
    //危险，内部虽然是线程安全，但是外部调用的时候就不安全了，如果两个线程判断都为空，都要去执行pop操作，当都判断成功
    //要取执行pop时就会出栈两次出现错误
    //解决办法：pop抛出异常，等执行pop的时候发现栈为空就抛出异常，外部捕获异常增加健壮性
    bool empty() const {
        std::lock_guard<std::mutex> lg(mutex);
        return data_.empty();
    }
}; 


//避免死锁就将加解锁的操作封装成一个独立的函数
//保证在独立的函数里执行完操作后就解锁，不会导致一个函数里使用多个锁的情况
//拷贝构造不用判断是否是自拷贝，拷贝赋值要判断是否自赋值
//实现移动拷贝和拷贝构造进行赋值操作的时候，如果没有重载赋值就不支持，重载了拷贝赋值就可以进行赋值，左值和右值都走拷贝赋值
//实现了移动赋值时右值才会走移动赋值
//没有实现移动构造还能移动默认走的是拷贝构造


//避免死锁，一次性获取所有的锁

class testobj {
public:
    testobj(int data) :data_(data) {}
    //拷贝构造
    testobj(const testobj& b2) :data_(b2.data_) {
        data_ = b2.data_;
    }
    //移动构造
    testobj(testobj&& b2) :data_(std::move(b2.data_)) {
    }
    //重载输出运算符
    friend std::ostream& operator << (std::ostream& os, const testobj& big_obj) {
        os << big_obj.data_;
        return os;
    }
    //重载赋值运算符
    testobj& operator = (const testobj& b2) {
        if (this == &b2) {
            return *this;
        }
        data_ = b2.data_;
        return *this;
    }
    //交换数据
    friend void swap(testobj& b1, testobj& b2) {
        testobj temp = std::move(b1);
        b1 = std::move(b2);
        b2 = std::move(temp);
    }
private:
    int data_;
};

class obj_mananger {
public:
    obj_mananger(int data = 0) :_obj(data) {}
    void printinfo() {
        std::cout << "current obj data is " << _obj << std::endl;
    }
    friend void danger_swap(obj_mananger& objm1, obj_mananger& objm2);
    friend void safe_swap(obj_mananger& objm1, obj_mananger& objm2);
    friend void safe_swap_scope(obj_mananger& objm1, obj_mananger& objm2);
private:
    std::mutex _mtx;
    testobj _obj;
};

void danger_swap(obj_mananger& objm1, obj_mananger& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::lock_guard <std::mutex> gurad1(objm1._mtx);
    //先获得一把锁等一会再去获取锁可能会死锁
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard<std::mutex> guard2(objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

void safe_swap(obj_mananger& objm1, obj_mananger& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    std::lock(objm1._mtx, objm2._mtx);
    //领养锁管理它自动释放，不管加锁只管解锁
    std::lock_guard <std::mutex> gurad1(objm1._mtx, std::adopt_lock);
    
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::lock_guard <std::mutex> gurad2(objm2._mtx, std::adopt_lock);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

//上述代码可以简化为以下方式
void safe_swap_scope(obj_mananger& objm1, obj_mananger& objm2) {
    std::cout << "thread [ " << std::this_thread::get_id() << " ] begin" << std::endl;
    if (&objm1 == &objm2) {
        return;
    }
    //同时加锁，c++17支持
    std::scoped_lock guard(objm1._mtx, objm2._mtx);
    //等价于
    //std::scoped_lock<std::mutex, std::mutex> guard(objm1._mtx, objm2._mtx);
    swap(objm1._obj, objm2._obj);
    std::cout << "thread [ " << std::this_thread::get_id() << " ] end" << std::endl;
}

//同时加锁对性能会有降低，如果一个锁加不上就会一直卡住
//层级锁
//确保所有的线程都是按照相同的顺序获得锁，那么死锁就不会发生,要求后加锁的先解锁
class hierarchical_mutex {
public:
    explicit hierarchical_mutex(unsigned long value) :_hierarchy_value(value),
        _previous_hierarchy_value(0) {}
    //删除拷贝构造和拷贝复制也不能移动了，因为没有定义移动函数
    hierarchical_mutex(const hierarchical_mutex&) = delete;
    hierarchical_mutex& operator=(const hierarchical_mutex&) = delete;
    //加锁，检测是否可以加锁，锁的层级是否比线程当前持有锁的层级高，值越低层级越高，
    //满足条件则允许加锁，更新当前线程锁的层级
    void lock() {
        check_for_hierarchy_violation();
        _internal_mutex.lock();
        update_hierarchy_value();
    }
    //解锁也需要按顺序，如果要解的锁不是线程当前层级则不允许解锁
    void unlock() {
        if (_this_thread_hierarchy_value != _hierarchy_value) {
            throw std::logic_error("mutex hierarchy violated");
        }
        _this_thread_hierarchy_value = _previous_hierarchy_value;
        _internal_mutex.unlock();
    }
    bool try_lock() {
        check_for_hierarchy_violation();
        if (!_internal_mutex.try_lock()) {
            return false;
        }
        update_hierarchy_value();
        return true;
    }
private:
    std::mutex  _internal_mutex;   
    unsigned long const _hierarchy_value;       //锁对象的层级
    unsigned long _previous_hierarchy_value;    //加锁前的层级
    static thread_local  unsigned long  _this_thread_hierarchy_value;   //线程局部静态变量，记录当前线程持有锁的最高层级
    void check_for_hierarchy_violation() {  //检查是否满足加锁条件
        if (_this_thread_hierarchy_value <= _hierarchy_value) {
            throw  std::logic_error("mutex  hierarchy violated");
        }
    }
    void  update_hierarchy_value() {    //更新层级
        _previous_hierarchy_value = _this_thread_hierarchy_value;
        _this_thread_hierarchy_value = _hierarchy_value;
    }
};
thread_local unsigned long hierarchical_mutex::_this_thread_hierarchy_value(ULONG_MAX);

void test_hierarchy_lock() {
    hierarchical_mutex  hmtx1(1000);
    hierarchical_mutex  hmtx2(500);
    std::thread t1([&hmtx1, &hmtx2]() {
        hmtx1.lock();
        hmtx2.lock();
        hmtx2.unlock();
        hmtx1.unlock();
        });
        
    std::thread t2([&hmtx1, &hmtx2]() {
        hmtx2.lock();
        hmtx1.lock();   //加锁失败，顺序不对
        hmtx1.unlock();
        hmtx2.unlock();
        });
    t1.join();
    t2.join();
}

//递归锁，加锁时候调用函数内部又加了这把锁，函数之间存在调用关系


//条件变量实现线程安全队列
template<typename T>
class queue_ts
{
private:
    std::mutex mtx;
    std::queue<T> data_queue;
    std::condition_variable data_cond;
public:
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mtx);
        data_queue.push(new_value);
        data_cond.notify_one();
    }
    //减少数据拷贝，防止第二次拷贝的时候崩溃
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        //front返回的是引用
        value = data_queue.front();
        data_queue.pop();
    }

    std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mtx);
		data_cond.wait(lk, [this] {return !data_queue.empty(); });
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
		data_queue.pop();
        //返回局部变量res引用外部有一个赋值操作，res的引用计数加一，出局部作用域后res析构引用计数减一
        //先赋值再结束函数调用吗
		return res;
	}
    //非阻塞pop，类似redis
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (data_queue.empty())
			return false;
		value = data_queue.front();
		data_queue.pop();
		return true;
	}
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (data_queue.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(data_queue.front()));
		data_queue.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mtx);
		return data_queue.empty();
	}
};


// int main() {

// }