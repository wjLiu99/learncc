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
    std::queue<T> data_;
    std::condition_variable cond_;
public:
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lk(mtx);
        data_.push(new_value);
        cond_.notify_one();
    }
    //减少数据拷贝，防止第二次拷贝的时候崩溃，阻塞pop
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mtx);
        cond_.wait(lk,[this]{return !data_.empty();});
        //front返回的是引用,给外部传进来的变量拷贝赋值
        value = data_.front();
        data_.pop();
    }

    std::shared_ptr<T> wait_and_pop()
	{
		std::unique_lock<std::mutex> lk(mtx);
		cond_.wait(lk, [this] {return !data_.empty(); });
		//使用对象构造智能指针失败的情况可能发生，因为makeshared会对数据进行拷贝，导致内存溢出
		//虽然不会丢失数据，但是数据滞留在队列里了不会被消费，解决办法1 有线程push发信号通知 2 notify all  3 队列存储智能指针
		std::shared_ptr<T> res(std::make_shared<T>(data_.front()));
		data_.pop();
        //返回局部变量res引用外部有一个赋值操作，res的引用计数加一，出局部作用域后res析构引用计数减一
    
		return res;
	}
    //非阻塞pop，类似redis，队列为空返回false
	bool try_pop(T& value)
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (data_.empty())
			return false;
		value = data_.front();
		data_.pop();
		return true;
	}
    //队列为空返回空智能指针
	std::shared_ptr<T> try_pop()
	{
		std::lock_guard<std::mutex> lk(mtx);
		if (data_.empty())
			return std::shared_ptr<T>();
		std::shared_ptr<T> res(std::make_shared<T>(data_.front()));
		data_.pop();
		return res;
	}
	bool empty() const
	{
		std::lock_guard<std::mutex> lk(mtx);
		return data_.empty();
	}
};


/*
 队列存储智能指针，在push的时候就使用参数构造智能指针，如果内存不足构造失败的话也不会污染队列里的数据
 智能指针在拷贝和赋值的时候引发异常的可能性极低，一个智能指针共享引用计数的情况下复制的开销很低
*/
template<typename T>
class threadsafe_queue_ptr
{
private:
    mutable std::mutex mtx_;
    std::queue<std::shared_ptr<T>> data_;
    std::condition_variable cond_;
public:
    threadsafe_queue_ptr()
    {}
    void wait_and_pop(T& value)
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cond_.wait(lk, [this] {return !data_.empty(); });
		//无锁队列使用的是自己的数据结构，手动开辟和构造，所以出队要手动析构，使用队列pop可以自动析构
        value = std::move(*data_.front());    
        data_.pop();
    }
    bool try_pop(T& value)
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (data_.empty())
            return false;
        value = std::move(*data_.front());  
            data_.pop();
        return true;
    }
    std::shared_ptr<T> wait_and_pop()
    {
        std::unique_lock<std::mutex> lk(mtx_);
        cond_.wait(lk, [this] {return !data_.empty(); });
        std::shared_ptr<T> res = data_.front();   
        data_.pop();
        return res;
    }
    std::shared_ptr<T> try_pop()
    {
        std::lock_guard<std::mutex> lk(mtx_);
        if (data_.empty())
            return std::shared_ptr<T>();
        std::shared_ptr<T> res = data_.front();   
        data_.pop();
        return res;
    }
    // void push(T new_value)
    // {
    //     std::shared_ptr<T> data(
    //         std::make_shared<T>(std::move(new_value)));   
    //         std::lock_guard<std::mutex> lk(mtx_);
    //     data_.push(data);
    //     cond_.notify_one();
    // }
	template<typename ...Args>
	void push(Args && ...new_value)
    {
        std::shared_ptr<T> data(
        std::make_shared<T>(std::forward<Args>(new_value)...));   
        std::lock_guard<std::mutex> lk(mtx_);
        data_.push(data);
        cond_.notify_one();
    }
    bool empty() const
    {
        std::lock_guard<std::mutex> lk(mtx_);
        return data_.empty();
    }
};
//还是没有实现入队出队并行
//使用链表实现并发队列可以实现push和pop并行

template<typename T>
class threadsafe_queue_l {
	struct node {
		std::shared_ptr<T> data_;
		std::shared_ptr<node> next_;
	};
public:

	//阻塞等待队列不空
	std::shared_ptr<T> wait_pop_head () {
		std::unique_lock<std::mutex> lk(head_mtx_);
		cond_.wait(lk, [&](){
			return head_.get() != get_tail().get();
		});

		// std::shared_ptr<node> d = std::move(head_);
		//拷贝数据
		std::shared_ptr<T> data = std::move(head_->data_);
		//移动头节点
		head_ = std::move(head_->next_);
		// head_ = head_->next_; 可以直接这样
		return data;
	}

	void wait_pop_head (T &val) {
		std::unique_lock<std::mutex> lk(head_mtx_);
		cond_.wait(lk, [&](){
			return head_.get() != get_tail().get();
		});

		val = std::move(*head_->data_);
		//head后移，当前节点的引用计数为0自动析构
		head_ = std::move(head_->next_);

	}

	std::shared_ptr<T> try_pop_head()
        {
            std::lock_guard<std::mutex> lk(head_mtx_);
            if (head_.get() == get_tail().get())
            {
                return std::shared_ptr<T>();
            }
            std::shared_ptr<node> d = std::move(head_);
			//智能指针的拷贝赋值和移动开销差不多
			head_ = std::move(d->next_);
			return d->data_;
        }

    bool try_pop_head(T& value)
        {
            std::lock_guard<std::mutex> lk(head_mtx_);
            if (head_.get() == get_tail().get())
            {
                return false;
            }
            value = std::move(*head_->data_);
			head_ = std::move(head_->next_);
			return true;
        }


public:
	threadsafe_queue_l () : head_(new node), tail_(head_) {}
	~threadsafe_queue_l () {}
	threadsafe_queue_l (const threadsafe_queue_l &) = delete;
	threadsafe_queue_l &operator= (const threadsafe_queue_l &) = delete;
	
	void push (T value) {
		//构造数据节点
		std::shared_ptr<T> new_data(std::make_shared<T>(std::move(value)));
		//构造虚位节点
		std::shared_ptr<node> new_node(new node);
		std::lock_guard<std::mutex> lk(tail_mtx_);
		tail_->data_ = new_data;
		tail_->next_ = new_node;
		tail_ = new_node;
		cond_.notify_one();
	}

	template<typename ...Args>
	void emplace (Args &&...arg) {
		//构造数据节点
		std::shared_ptr<T> new_data(std::make_shared<T>(std::forward<Args>(arg)...));
		//构造虚位节点
		std::shared_ptr<node> new_node(new node);
		std::lock_guard<std::mutex> lk(tail_mtx_);
		tail_->data_ = new_data;
		tail_->next_ = new_node;
		tail_ = new_node;
		cond_.notify_one();
	}
private:


	std::mutex head_mtx_;
	std::mutex tail_mtx_;

	std::shared_ptr<node> head_;
	std::shared_ptr<node> tail_; //尾部指针需要共享

	std::condition_variable cond_;

	std::shared_ptr<node> get_tail () {
		std::lock_guard<std::mutex> lk(tail_mtx_);
		return tail_;
	}



};
/*
二次析构问题：头结点uniqueptr 尾节点sharedptr，因为尾节点需要多个指针指向，开始的想法是用头结点的裸指针构造尾节点，然后尾节点往后移动不影响，但是插入新节点的时候
连接好next后尾节点往后移，当前节点的这个共享指针的引用计数就位0了，导致当前节点析构，但是头结点的独占指针还指向该节点，最后后造成二次析构问题
错误：将同一资源交给两个智能指针管理
另一设计思路：头结点和next节点都用独占指针，尾节点直接用裸指针
*/



#endif