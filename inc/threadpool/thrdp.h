#ifndef _THREADPOOL_H
#define _THREADPOOL_H

#include <iostream>
#include <vector>
#include <queue>
#include <memory>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <unordered_map>
#include <thread>
#include <future>

#define TASK_MAX_THRESHHOLD  2  // INT32_MAX
#define THREAD_MAX_THRESHHOLD  1024
#define THREAD_MAX_IDLE_TIME  60 // 单位：秒



/*
	线程池不适合的场景
	1 任务之间有顺序
	2 任务之间互斥，或者强关联 如进工会加贡献值和会长消耗贡献值，可以加锁实现但是加锁开销很大不如一个线程执行
	如redis，nginx处理逻辑的时候都是单线程，多线程也只在网络部分
*/


// 线程池支持的模式
enum pool_mode
{
	MODE_FIXED,  // 固定数量的线程
	MODE_CACHED, // 线程数量可动态增长
};

// 线程类型
class thread_m
{
public:
	// 线程函数对象类型
	using thread_func = std::function<void(int)>;

	// 线程构造
	thread_m(thread_func func)
		: func_(func)
		, thread_id_(generate_id_++)
	{}
	// 线程析构
	~thread_m() = default;

	// 启动线程
	void start()
	{
		// 创建一个线程来执行一个线程函数 pthread_create
		std::thread t(func_, thread_id_);  // C++11来说 线程对象t  和线程函数func_
		t.detach(); // 设置分离线程   pthread_detach  pthread_t设置成分离线程
	}

	// 获取线程id
	int get_id() const
	{
		return thread_id_;
	}
private:
	thread_func func_;
	static int generate_id_;
	int thread_id_;  // 保存线程id
};

int thread_m::generate_id_ = 0;

// 线程池类型
class thread_pool
{
public:
	// 线程池构造
	thread_pool()
		: init_size_(0)
		, task_size_(0)
		, idle_thread_cnt_(0)
		, cur_thread_cnt_(0)
		, task_maxsize_(TASK_MAX_THRESHHOLD)
		, thread_maxsize_(THREAD_MAX_THRESHHOLD)
		, mode_(MODE_FIXED)
		, running_(false)
	{}

	// 线程池析构
	~thread_pool()
	{
		running_ = false;

		// 等待线程池里面所有的线程返回  有两种状态：阻塞 & 正在执行任务中
		std::unique_lock<std::mutex> lock(mtx_);
		not_empty_.notify_all();
		exitCond_.wait(lock, [&]()->bool {return threads_.size() == 0; });
	}

	// 设置线程池的工作模式
	void set_mode(pool_mode mode)
	{
		if (check_state())
			return;
		mode_ = mode;
	}

	// 设置task任务队列上线阈值
	void set_task_maxsize(int threshhold)
	{
		if (check_state())
			return;
		task_maxsize_ = threshhold;
	}

	// 设置线程池cached模式下线程阈值
	void set_thread_maxsize(int size)
	{
		if (check_state())
			return;
		if (mode_ == MODE_CACHED)
		{
			thread_maxsize_ = size;
		}
	}

	// 给线程池提交任务
	// 使用可变参模板编程，让submitTask可以接收任意任务函数和任意数量的参数
	// pool.commit(sum1, 10, 20);  右值引用+引用折叠原理
	// 返回值future<>
	template<typename Func, typename... Args>
	auto commit(Func&& func, Args&&... args) -> std::future<decltype(func(args...))>
	{
		// 打包任务，放入任务队列里面,通过表达式推导返回值类型
		using return_type = decltype(func(args...));
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<Func>(func), std::forward<Args>(args)...));
		std::future<return_type> result = task->get_future();

		// 获取锁
		std::unique_lock<std::mutex> lock(mtx_);
		// 用户提交任务，最长不能阻塞超过1s，否则判断提交任务失败，返回
		if (!not_full_.wait_for(lock, std::chrono::seconds(1),
			[&]()->bool { return task_queue_.size() < (size_t)task_maxsize_; }))
		{
			// 表示notFull_等待1s种，条件依然没有满足
			std::cerr << "task queue is full, submit task fail." << std::endl;
			auto task = std::make_shared<std::packaged_task<return_type()>>(
				[]()->return_type { return return_type(); });
			(*task)();
			return task->get_future();
		}

		// 如果有空余，把任务放入任务队列中
		// task_queue_.emplace(sp);  
		// using Task = std::function<void()>;
        // 以值捕获智能指针会增加引用计数，千万不要捕获局部变量的引用，这样不能延长变量的生命周期，对象释放后再调用就会崩溃
		// task解引用是packaged_task类型，重载了（）运算符
		task_queue_.emplace([task]() {(*task)();});
		task_size_++;

		// 因为新放了任务，任务队列肯定不空了，在notEmpty_上进行通知，赶快分配线程执行任务
		not_empty_.notify_all();

		// cached模式 任务处理比较紧急 场景：小而快的任务 需要根据任务数量和空闲线程的数量，判断是否需要创建新的线程出来
		if (mode_ == MODE_CACHED
			&& task_size_ > idle_thread_cnt_
			&& cur_thread_cnt_ < thread_maxsize_)
		{
			std::cout << ">>> create new thread..." << std::endl;

			// 创建新的线程对象
			auto ptr = std::make_unique<thread_m>(std::bind(&thread_pool::thread_func, this, std::placeholders::_1));
			int threadId = ptr->get_id();
			threads_.emplace(threadId, std::move(ptr));
			// 启动线程
			threads_[threadId]->start();
			// 修改线程个数相关的变量
			cur_thread_cnt_++;
			idle_thread_cnt_++;
		}

		// 返回任务的Result对象
		return result;
	}

	// 开启线程池
	void start(int init_size = std::thread::hardware_concurrency())
	{
		// 设置线程池的运行状态
		running_ = true;

		// 记录初始线程个数
		init_size_ = init_size;
		cur_thread_cnt_ = init_size;

		// 创建线程对象
		for (int i = 0; i < init_size_; i++)
		{
			// 创建thread线程对象的时候，把线程函数给到thread线程对象
			auto ptr = std::make_unique<thread_m>(std::bind(&thread_pool::thread_func, this, std::placeholders::_1));
			int threadId = ptr->get_id();
			threads_.emplace(threadId, std::move(ptr));
			// threads_.emplace_back(std::move(ptr));
		}

		// 启动所有线程  std::vector<thread_m*> threads_;
		for (int i = 0; i < init_size_; i++)
		{
			threads_[i]->start(); // 需要去执行一个线程函数
			idle_thread_cnt_++;    // 记录初始空闲线程的数量
		}
	}

	thread_pool(const thread_pool&) = delete;
	thread_pool& operator=(const thread_pool&) = delete;

private:
	// 定义线程函数
	void thread_func(int threadid)
	{
		auto last_time = std::chrono::high_resolution_clock().now();

		// 所有任务必须执行完成，线程池才可以回收所有线程资源
		for (;;)
		{
			Task task;
			{
				// 先获取锁
				std::unique_lock<std::mutex> lock(mtx_);

				std::cout << "tid:" << std::this_thread::get_id()
					<< "尝试获取任务..." << std::endl;

				// cached模式下，有可能已经创建了很多的线程，但是空闲时间超过60s，应该把多余的线程
				// 结束回收掉（超过initThreadSize_数量的线程要进行回收）
				// 当前时间 - 上一次线程执行的时间 > 60s

				// 每一秒中返回一次   怎么区分：超时返回？还是有任务待执行返回
				// 锁 + 双重判断
				while (task_queue_.size() == 0)
				{
					// 线程池要结束，回收线程资源
					if (!running_)
					{
						threads_.erase(threadid); // std::this_thread::getid()
						std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
							<< std::endl;
						exitCond_.notify_all();
						return; // 线程函数结束，线程结束
					}

					if (mode_ == MODE_CACHED)
					{
						// 条件变量，超时返回了
						if (std::cv_status::timeout ==
							not_empty_.wait_for(lock, std::chrono::seconds(1)))
						{
							auto now = std::chrono::high_resolution_clock().now();
							auto dur = std::chrono::duration_cast<std::chrono::seconds>(now - last_time);
							if (dur.count() >= THREAD_MAX_IDLE_TIME
								&& cur_thread_cnt_ > init_size_)
							{
								// 开始回收当前线程
								// 记录线程数量的相关变量的值修改
								// 把线程对象从线程列表容器中删除   没有办法 threadFunc《=》thread对象
								// threadid => thread对象 => 删除
								threads_.erase(threadid); // std::this_thread::getid()
								cur_thread_cnt_--;
								idle_thread_cnt_--;

								std::cout << "threadid:" << std::this_thread::get_id() << " exit!"
									<< std::endl;
								return;
							}
						}
					}
					else
					{
						// 等待notEmpty条件
						not_empty_.wait(lock);
					}
				}

				idle_thread_cnt_--;

				std::cout << "tid:" << std::this_thread::get_id()
					<< "获取任务成功..." << std::endl;

				// 从任务队列种取一个任务出来
				task = task_queue_.front();
				task_queue_.pop();
				task_size_--;

				// 如果依然有剩余任务，继续通知其它得线程执行任务
				if (task_queue_.size() > 0)
				{
					not_empty_.notify_all();
				}

				// 取出一个任务，进行通知，通知可以继续提交生产任务
				not_full_.notify_all();
			} // 就应该把锁释放掉

			// 当前线程负责执行这个任务
			if (task != nullptr)
			{
				task(); // 执行function<void()> 
			}

			idle_thread_cnt_++;
			last_time = std::chrono::high_resolution_clock().now(); // 更新线程执行完任务的时间
		}
	}

	// 检查pool的运行状态
	bool check_state() const
	{
		return running_;
	}

private:
	std::unordered_map<int, std::unique_ptr<thread_m>> threads_; // 线程列表

	int init_size_;  // 初始的线程数量
	int thread_maxsize_; // 线程数量上限阈值
	std::atomic_int cur_thread_cnt_;	// 记录当前线程池里面线程的总数量
	std::atomic_int idle_thread_cnt_; // 记录空闲线程的数量

	// Task任务 =》 函数对象
	using Task = std::function<void()>;
	std::queue<Task> task_queue_; // 任务队列
	std::atomic_int task_size_; // 任务的数量
	int task_maxsize_;  // 任务队列数量上限阈值

	std::mutex mtx_; // 保证任务队列的线程安全
	std::condition_variable not_full_; // 表示任务队列不满
	std::condition_variable not_empty_; // 表示任务队列不空
	std::condition_variable exitCond_; // 等到线程资源全部回收

	pool_mode mode_; // 当前线程池的工作模式
	std::atomic_bool running_; // 表示当前线程池的启动状态
};

#endif
