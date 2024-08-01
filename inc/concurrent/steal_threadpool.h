#ifndef _STEAL_THREADPOOL_H_
#define _STEAL_THREADPOOL_H_
#include "threadsafe_queue.h"
#include <future>
#include <vector>

/*
	任务窃取线程池实现
	只能使用轮询方式实现，阻塞在条件变量上就无法达到任务窃取的效果
	多任务队列避免了资源竞争
	线程自动汇合

*/
class join_threads
{
    std::vector<std::thread>& threads;
public:
    explicit join_threads(std::vector<std::thread>& threads_) :
        threads(threads_)
    {}
    ~join_threads()
    {
        for (unsigned long i = 0; i < threads.size(); ++i)
        {
            if (threads[i].joinable())
                threads[i].join();
        }
    }
};



class function_wrapper
{
	struct impl_base {
		virtual void call() = 0;
		virtual ~impl_base() {}
	};
	std::unique_ptr<impl_base> impl;
	template<typename F>
	struct impl_type : impl_base
	{
		F f;
		impl_type(F&& f_) : f(std::move(f_)) {}
		void call() { f(); }
	};
public:
	template<typename F>
	function_wrapper(F&& f) :
		impl(new impl_type<F>(std::move(f)))
	{}
	void operator()() { impl->call(); }
	function_wrapper() = default;
	function_wrapper(function_wrapper&& other) :
		impl(std::move(other.impl))
	{}
	function_wrapper& operator=(function_wrapper&& other)
	{
		impl = std::move(other.impl);
		return *this;
	}
	function_wrapper(const function_wrapper&) = delete;
	function_wrapper(function_wrapper&) = delete;
	function_wrapper& operator=(const function_wrapper&) = delete;
};


class steal_thread_pool
{
private:

	void worker_thread(int index)
	{
		while (!done)
		{
			function_wrapper wrapper;
			bool pop_res = thread_work_ques[index].try_pop(wrapper);
			if (pop_res) {
				wrapper();
				continue;
			}

			bool steal_res = false;
			for (int i = 0; i < thread_work_ques.size(); i++) {
				if (i == index) {
					continue;
				}

				steal_res  = thread_work_ques[i].try_steal(wrapper);
				if (steal_res) {
					wrapper();
					break;
				}

			}

			if (steal_res) {
				continue;
			}
			
			std::this_thread::yield();
		}
	}
public:

	static steal_thread_pool& instance() {
		static  steal_thread_pool pool;
		return pool;
	}
	~steal_thread_pool()
	{

		done = true;
		for (unsigned i = 0; i < thread_work_ques.size(); i++) {
			thread_work_ques[i].Exit();
		}

		for (unsigned i = 0; i < threads.size(); ++i)
		{

			threads[i].join();
		}
	}

	template<typename FunctionType>
	std::future<typename std::result_of<FunctionType()>::type>
		submit(FunctionType f)
	{
		int index = (atm_index.load() + 1) % thread_work_ques.size();
		atm_index.store(index);
		typedef typename std::result_of<FunctionType()>::type result_type;
		std::packaged_task<result_type()> task(std::move(f));
		std::future<result_type> res(task.get_future());
		thread_work_ques[index].push(std::move(task));
		return res;
	}

private:
	steal_thread_pool() :
		done(false), joiner(threads), atm_index(0)
	{

		unsigned const thread_count = std::thread::hardware_concurrency();
		try
		{
			thread_work_ques = std::vector < threadsafe_queue_s<function_wrapper>>(thread_count);

			for (unsigned i = 0; i < thread_count; ++i)
			{

				threads.push_back(std::thread(&steal_thread_pool::worker_thread, this, i));
			}
		}
		catch (...)
		{

			done = true;
			for (int i = 0; i < thread_work_ques.size(); i++) {
				thread_work_ques[i].Exit();
			}
			throw;
		}
	}

	std::atomic_bool done;
	//全局队列
	std::vector<threadsafe_queue_s<function_wrapper>> thread_work_ques;
	std::vector<std::thread> threads;
	join_threads joiner;
	std::atomic<int>  atm_index;
};

#endif