#ifndef _TS_QUEUE_H_
#define _TS_QUEUE_H_

#include <mutex>
#include <condition_variable>
#include <queue>

//条件变量实现线程安全队列
template<typename T>
class threadsafe_queue
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
    //减少数据拷贝，防止第二次拷贝的时候崩溃，阻塞pop
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mtx);
        data_cond.wait(lk,[this]{return !data_queue.empty();});
        //front返回的是引用,给外部传进来的变量拷贝赋值
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
    
		return res;
	}
    //非阻塞pop，类似redis，队列为空返回false
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (data_queue.empty())
			return false;
		value = data_queue.front();
		data_queue.pop();
		return true;
	}
    //队列为空返回空智能指针
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


#endif