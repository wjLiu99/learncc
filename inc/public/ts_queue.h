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



//无锁队列实现
template<typename T>
class lock_free_queue
{
private:
    //线程进入线程外部计数+1，线程离开，内部引用计数-1
    struct node_counter
    {
        unsigned internal_count : 30;
        // 外部指针计数，最大值为2，记录节点的next指针和tail指针
        unsigned external_counters : 2;
    };

    struct node;

    struct counted_node_ptr
    {

        //构造初始化各成员
        counted_node_ptr():external_count(0), ptr(nullptr) {}
        int external_count;
        node* ptr;
    };

    struct node
    {
        std::atomic<T*> data;
        std::atomic<node_counter> count;
        //⇽---  1
        std::atomic<counted_node_ptr> next;

        node(int external_count = 2)
        {
            node_counter new_count;
            new_count.internal_count = 0;
            //⇽---  4
            new_count.external_counters = external_count;
            count.store(new_count);

            counted_node_ptr node_ptr;
			node_ptr.ptr = nullptr;
			node_ptr.external_count = 0;

            next.store(node_ptr);
        }

        //减少内部计数，为离开的线程数
        void release_ref()
        {
            node_counter old_counter =
                count.load(std::memory_order_relaxed);
            node_counter new_counter;
            do
            {
                new_counter = old_counter;
                --new_counter.internal_count;
            }
            while (!count.compare_exchange_strong(
                old_counter, new_counter,
                std::memory_order_acquire, std::memory_order_relaxed));
            // 满足条件删除节点
            if (!new_counter.internal_count &&
                !new_counter.external_counters)
            {
                delete this
            }
        }
    };

    std::atomic<counted_node_ptr> head;

    std::atomic<counted_node_ptr> tail;

    // 设置新的尾节点
    void set_new_tail(counted_node_ptr& old_tail,
        counted_node_ptr const& new_tail)
    {
        node* const current_tail_ptr = old_tail.ptr;
        // 此处仅有一个线程能设置tail为new_tail，失败的会更新old_tail为tail的新值
        // 为防止失败的线程重试导致tail被再次更新所以添加了后面的&&判断，这时候尾部的node节点已经不是原来的node节点了，会跳出循环
		//如果tail和old_tail不等说明引用计数不同或者tail已经被移动，如果tail已经被移动那么old_tail的ptr和current_tail_ptr不同，则可以直接退出。
		//所以一旦tail被设置为new_tail，那么另一个线程在重试时判断tail和old_tail不等，会修改old_tail, 此时old_tail已经和current_tail不一致了，所以没必要再重试。
       //如不加后续判断， 会造成重复设置newtail，引发多插入节点的问题。
        while (!tail.compare_exchange_weak(old_tail, new_tail) &&
            old_tail.ptr == current_tail_ptr);
        // 设置新的尾节点成功
        if (old_tail.ptr == current_tail_ptr)
            //插入成功，减少外部指针计数和外部线程计数
            free_external_counter(old_tail);
        else
            //插入失败，减少内部计数
            current_tail_ptr->release_ref();
    }
    //外部指针 -1 内部计数 + 正在操作的线程数
    static void free_external_counter(counted_node_ptr& old_node_ptr)
    {
        node* const ptr = old_node_ptr.ptr;
        int const count_increase = old_node_ptr.external_count - 2;
        node_counter old_counter =
            ptr->count.load(std::memory_order_relaxed);
        node_counter new_counter;
        do
        {
            new_counter = old_counter;
            // 减少外部指针数
            --new_counter.external_counters;
            // 操作线程数内部计数增加
            new_counter.internal_count += count_increase;
        }
        // 需要替换整个count节点
        while (!ptr->count.compare_exchange_strong(
            old_counter, new_counter,
            std::memory_order_acquire, std::memory_order_relaxed));
            // 如果指针外部计数和操作线程计数为0可以删除节点
        if (!new_counter.internal_count &&
            !new_counter.external_counters)
        {
            // 删除该节点
            delete ptr;
        }

    }

    //增加node_ptr中的外部引用计数，为操作该节点的线程数
    static void increase_external_count(
        std::atomic<counted_node_ptr>& counter,
        counted_node_ptr& old_counter)
    {
        counted_node_ptr new_counter;
        do
        {
            new_counter = old_counter;
            ++new_counter.external_count;
        } while (!counter.compare_exchange_strong(
            old_counter, new_counter,
            std::memory_order_acquire, std::memory_order_relaxed));
        old_counter.external_count = new_counter.external_count;
    }

public:
    lock_free_queue() {
       
		counted_node_ptr new_next;
		new_next.ptr = new node();
		new_next.external_count = 1;
		tail.store(new_next);
		head.store(new_next);
    }

    ~lock_free_queue() {
        while (pop());
        auto head_counted_node = head.load();
        delete head_counted_node.ptr;
    }

    void push(T new_value)
    {
        std::unique_ptr<T> new_data(new T(new_value));
        counted_node_ptr new_next;
        new_next.ptr = new node;
        new_next.external_count = 1;
        counted_node_ptr old_tail = tail.load();
        for (;;)
        {
            //增加外部线程计数
            increase_external_count(tail, old_tail);
            T* old_data = nullptr;
            //设置尾节点的数据域
            if (old_tail.ptr->data.compare_exchange_strong(
                old_data, new_data.get()))
            {
                counted_node_ptr old_next;
                counted_node_ptr now_next = old_tail.ptr->next.load();
                // 链接新的节点，可能失败是因为别的节点辅助完成了新节点的设置，新的尾节点是一个空的节点，所以任何线程设置都可以
                if (!old_tail.ptr->next.compare_exchange_strong(
                    old_next, new_next))
                {
                    // 设置失败删除新节点
                    delete new_next.ptr;
                    new_next = old_next;   
                }
                //设置成功释放指针引用计数和线程引用计数，失败则释放线程引用计数
                set_new_tail(old_tail, new_next);
                //释放智能指针指向的资源但是不析构
                new_data.release();
                break;
            }
            else    // 尾节点数据设置失败，辅助另一线程push操作，更新尾节点
            {
                counted_node_ptr old_next ;
     
                if (old_tail.ptr->next.compare_exchange_strong(
                    old_next, new_next))
                {
                    old_next = new_next;
                    new_next.ptr = new node;
                }
                set_new_tail(old_tail, old_next);
            }
        }

    }


    std::unique_ptr<T> pop()
    {
        counted_node_ptr old_head = head.load(std::memory_order_relaxed);
            for (;;)
            {
                increase_external_count(head, old_head);
                node* const ptr = old_head.ptr;
                if (ptr == tail.load().ptr)
                {
                    //头尾相等说明队列为空，要减少内部引用计数，线程退出
                    ptr->release_ref();
                    return std::unique_ptr<T>();
                }
                //  队列非空
                counted_node_ptr next = ptr->next.load();
                if (head.compare_exchange_strong(old_head, next))
                {
                    T* const res = ptr->data.exchange(nullptr);
                    //减少指针计数和线程计数
                    free_external_counter(old_head);
                    return std::unique_ptr<T>(res);
                }
                //减少线程计数
                ptr->release_ref();
            }
    }


};

/*
    为什么需要外部指针计数是因为pop可能会操作到尾节点，
    无锁栈中push不需要增加引用计数
    无锁队列中由于push和pop在不同的节点，但pop可能操作到正在push的节点，
    如果push已经设置好数据域还没有链接新节点指针，这时候是可以pop该数据了，如果push的时候不增加引用计数pop就有可能会删除掉该节点
    但是push时增加引用计数又不能和无锁栈一样操作完毕减少内部引用计数然后判断删除节点，因为push的时候肯定不能删除节点
    所以要添加一个新的给尾节点push引用计数的引用计数，该计数不为0也不能删除节点，push退出后原来线程的引用计数会为0但是新的外部指针引用计数不为0也是不会删除节点的
*/


#endif