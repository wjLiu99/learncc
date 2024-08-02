#ifndef _TS_STACK_H_
#define _TS_STACK_H_
#include <stdexcept> // 引入标准异常类
#include <memory>
#include <atomic>
#include <stack>
#include <mutex>

struct empty_stack : std::exception
{
    const char* what() const noexcept override {
        return "Empty stack exception";
    }
};
template<typename T>
class threadsafe_stack
{
private:
    std::stack<T> data;
    mutable std::mutex m;
public:
    threadsafe_stack() {}
    threadsafe_stack(const threadsafe_stack& other)
    {
        std::lock_guard<std::mutex> lock(other.m);
        //在构造函数的函数体内进行复制操作
        data = other.data;   
    }
    threadsafe_stack& operator=(const threadsafe_stack&) = delete;
    //此处使用拷贝构造，考虑到空间不足可能会丢失数据，可以采用vector判断size和capacity大小，相同就手动扩容保证数据安全
    void push(T new_value)
    {
        std::lock_guard<std::mutex> lock(m);
        data.push(std::move(new_value));
    }
    std::shared_ptr<T> pop()
    {
        std::lock_guard<std::mutex> lock(m);
        //出栈检查是否为空，为空返回空指针
        if (data.empty()) return nullptr;
        //改动栈容器前设置返回值，使用对象构造智能指针需要拷贝一份，可能会导致空间不足，但是不会丢数据
            std::shared_ptr<T> const res(std::make_shared<T>(data.top()));    
            data.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        // pop空栈返回空栈异常
        if (data.empty()) throw empty_stack();
        value = data.top(); //拷贝赋值
        data.pop();
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

//线程安全的无锁栈
template<typename T>
class threadsafe_stack_uk {
    // struct node {
    //     std::shared_ptr<T> data_;
    //     std::unique_ptr<node> next_;
    //     node (const T &v) : data_(std::make_shared<T>(v)) {}
    // };
    struct node
    {
        std::shared_ptr<T> data_;
        node *next_;
        node(const T &data) : 
                data_(std::make_shared<T>(data)), next_(nullptr)
        {}
    };

    threadsafe_stack_uk(const threadsafe_stack_uk&) = delete;
	threadsafe_stack_uk& operator = (const threadsafe_stack_uk&) = delete;
	std::atomic<node*> head_;
	std::atomic<node*> to_be_deleted;
	std::atomic<int> threads_in_pop;

public:
    //忘记初始化head为nullptr
    threadsafe_stack_uk () : head_(nullptr){}

    void push (const T &value) {
        node *new_node = new node(value);
        do {
            new_node->next_ = head_.load();
        } while (!head_.compare_exchange_weak(new_node->next_, new_node));
    }

    std::shared_ptr<T> pop() {
        ++threads_in_pop;
        node  *old = nullptr;
        //需要判断取出的节点目前还是不是head，有可能head已经被别的线程修改
        do {
            // node *old = head_.load();
            old = head_.load();
            //空栈
            if (nullptr == old) {
                --threads_in_pop;
                return nullptr;
            }
        } while(!head_.compare_exchange_weak(old, old->next_));
        //拷贝赋值可能出现异常，此时数据已经出栈造成数据丢失，锁版本不会丢失时因为先拷贝再出栈，修改为指针指针的拷贝降低异常风险
        // value = old->data_;
        // return old->data_;
        //未释放弹出节点的内存
        std::shared_ptr<T> res;
        if (old) {  
            res.swap(old->data_);
        }
        
        // delete old;     //此处存在巨大问题，如果把old节点delete了，其他还在使用old进行比较交换，old->next就会异常，必须增加延时删除机制
        try_reclaim(old);
        return res;

    }

    /*
        当只有一个线程pop的时候可以直接删除old节点
        当取出待删列表后只有一个线程执行pop可以删除待删列表
        如果取出待删列表后发现pop线程数不止一个不可以删除待删列表，需要将待删列表重新链接回去
        因为此的是待删列表中可能有后来的pop线程添加的节点，又被后续的pop线程所引用
        还是存在隐患，如果一直有线程pop就会一直得不到删除
    */
    void try_reclaim(node *old) {
        if (threads_in_pop == 1) {
            //局部变量换出待删节点
            node *to_delete = to_be_deleted.exchange(nullptr);
            //取出后pop线程数还为1，使用--获取精确的值
            if (!--threads_in_pop) {
                delete_nodes(to_delete);
             
            }
            //线程数不为1，归还待删节点链表
            else if (to_delete) {
                chain_pending_nodes(to_delete);
            }
            delete old;
            return;

        }

        //将old节点加入待删列表
        chain_pending_node(old);
        --threads_in_pop;

    }

    void delete_nodes(node *n) {
        while (n) {
            node *next = n->next_;
            delete n;
            n = next;
        }
    }

    void chain_pending_node(node *n) {
        return chain_pending_nodes(n, n);   
    }

    void chain_pending_nodes(node *first, node *last) {
        last->next_ = to_be_deleted;
        while (!to_be_deleted.compare_exchange_weak(last->next_, first));
    }

    void chain_pending_nodes(node *n) {
        node *last = n;
        while (last->next_) {
            last = last->next_;
           
        }
        return chain_pending_nodes(n, last);
    }

};



template<typename T>
class ref_count_stack {
private:
	//前置声明节点类型
	struct count_node;
	struct counted_node_ptr {
		//1 外部引用计数
		int external_count;
		//2 节点地址
		count_node* ptr;
	};

	struct count_node {
		//3  数据域智能指针
		std::shared_ptr<T> data;
		//4  节点内部引用计数
		std::atomic<int>  internal_count;
		//5  下一个节点
		counted_node_ptr  next;
		count_node(T const& data_): data(std::make_shared<T>(data_)), internal_count(0) {}
	};
	//6 头部节点
	std::atomic<counted_node_ptr> head;

public:
	//增加头部节点引用数量
	void increase_head_count(counted_node_ptr& old_counter) {
		counted_node_ptr new_counter;
		
		do {
			new_counter = old_counter;
			++new_counter.external_count;
		}//7  循环判断保证head和old_counter想等时做更新,多线程情况保证引用计数原子递增。
		while (!head.compare_exchange_strong(old_counter,  new_counter, 
			std::memory_order_acquire, std::memory_order_relaxed));
		//8  走到此处说明head的external_count已经被更新了
		old_counter.external_count = new_counter.external_count;
	}
    /*
        线程进入增加外部计数，退出减少内部计数
        最后一个离开的线程负责删除
    */
	std::shared_ptr<T> pop() {
		counted_node_ptr old_head = head.load();
		for (;;) {
			increase_head_count(old_head);
			count_node* const ptr = old_head.ptr;
			//1  判断为空责直接返回
			if (!ptr) {
				return std::shared_ptr<T>();
			}

			// 只有一个线程能修改成功进入
			if (head.compare_exchange_strong(old_head, ptr->next, std::memory_order_relaxed)) {
				//返回头部数据
				std::shared_ptr<T> res(nullptr);
				//交换数据
				res.swap(ptr->data);
                if (res == nullptr) {
                    throw empty_stack();
                }

				//3 减少外部引用计数，先统计到目前为止增加了多少外部引用
				int const count_increase = old_head.external_count - 2;
				// 修改成功的线程增加内部引用计数
				if (ptr->internal_count.fetch_add(count_increase, std::memory_order_release) == -count_increase) {
					delete  ptr;
                    
				}

          

                
				return res;
			} else if (ptr->internal_count.fetch_add(-1, std::memory_order_acquire) == 1) { //其余没有修改成功的线程都进入这里，减少内部引用计数，减为0时删除
            
				//如果当前线程操作的head节点已经被别的线程更新，则减少内部引用计数
				//当前线程减少内部引用计数，返回之前值为1说明指针仅被当前线程引用
				ptr->internal_count.load(std::memory_order_acquire);
        
				delete ptr;
			}
            
		}
	}

	ref_count_stack(){
		counted_node_ptr head_node_ptr;
		//头节点开始只做标识用，没效果
		head_node_ptr.external_count = 0;
		head_node_ptr.ptr = nullptr;
		head.store(head_node_ptr);
	}

	~ref_count_stack() {
		//循环出栈
		while (pop());
	}

	void push(T const& data) {
		counted_node_ptr  new_node;
		new_node.ptr = new count_node(data);
		new_node.external_count = 1;
		new_node.ptr->next = head.load();
		while (!head.compare_exchange_weak(new_node.ptr->next, new_node, 
			std::memory_order_release, std::memory_order_relaxed));
	}

	
};




/*

如果有多个不同线程的push引起的释放序列同时执行，不同线程之间的release没有明确的同步关系
释放序列和同步并没有直接关系，但是我们可以通过重试和对应的逻辑判断构成同步
只要获取操作看到的都是正确存储的节点就可以了，具体哪个线程先push，谁先pop是无所谓

只有seq_cst以及acquire release等模型可以达到同步，至于同一个原子变量的读改写是因为原子变量的原子性保证了写的安全。
释放序列中非acquire读改写操作的都是同一个原子变量构成了释放序列，释放序列中的relaxed等并不能保证编排顺序
*/

/*
    释放序列，如果线程读到了释放序列中某个读改写或者写的值，那么该读操作就与释放序列release为首的写操作同步
    对m写以release为首的释放序列，同线程中对m的写操作（任何内存序），不同线程对m的读改写操作（任何内存序）
    释放序列中的操作没有同步关系
    处于一个释放序列的操作中，都可以读到前面修改的最新值然后修改
*/

/*
如果一个经过适当标记的写操作W 同步于（synchronizes-with） 一个经过适当标记的读操作读R操作，则需要满足下列条件之一：

读到的是之前写操作W写入的值
读到的是写操作W所在的线程后续写操作写入的值
读到的值是任意线程在写操作W之后，Read-Modify-Write操作写入的值

    先读到前面操作的值才能构成同步，构成同步才能用先行关系进行推断
    人为的控制同步，加判断条件

    宽松内存序（‌std::memory_order_relaxed）‌是C++内存模型中最宽松的一种内存序，‌它允许最大的重排序。‌在同一个线程内部，‌如果先store了一个值，‌后续的load操作确实能看到这个store的值，‌这是因为它们在同一个线程中具有happens-before关系。‌但是，‌对于不同的线程，‌宽松内存序并不保证读取值的同步。‌这意味着，‌如果一个线程已经读取到某个值a，‌后续的load操作不能读取到比a更老的值，‌但这并不意味着能够读取到最新的值。‌

为了确保能够读取到最新的值，‌需要使用更严格的内存序，‌如顺序一致性内存序（‌std::memory_order_seq_cst）‌或者获取-释放内存序（‌std::memory_order_acquire和std::memory_order_release）‌。‌这些更严格的内存序提供了线程间的happens-before关系，‌确保了操作的顺序和可见性，‌从而能够读取到最新的值
*/

/*

可以在双引用的基础上做一下改造吗？在原本的数据结构上，不使用外部引用。将外部引用+1的行为变为直接增加内部引用数量。
这样各个线程能看到这个内部引用。执行完直接减1直到减为0最后删除数据
无所并发需要重试，如果都使用内部引用，其他线程做重试得时候会导致互相影响。
*/

/*
同步是一种先行，同步还包括上下文的关系，先行不止同步一种 先行是先执行可以看到结果 
relaxed不能读到最新结果但是读改写呢
relaxed内存序就算用逻辑控制两个操作同步了也不能保证操作的上文已经执行完毕，操作的下文还没执行
release acquire内存序如果acquire读到了release的值就构成了同步，release上文执行完毕，acquire下文还未执行
是先读到值才构成同步，如果b操作读到了a操作的值，就叫做a同步与b
释放序列，如果acquire读到了释放序列中任何一个写或者读改写的值就与释放序列第一个release操作同步
读可能给是旧值，但是写和读改写都是原子的，load的内存序只是限制指令编排和cpu执行的顺序
*/

/*
多线程对同一原子变量的修改有一个全局的顺序表，同一时刻不同线程读到的值可能不同，但是读取的值不能打破这个顺序
写的时候会往这个顺序表后面添加，单线程对同一原子变量是有先行关系的
*/
#endif