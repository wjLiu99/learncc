#ifndef _SINGLETON_H_
#define _SINGLETON_H_
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>

class SingleTon {
private:
    SingleTon () {}
    SingleTon (const SingleTon&) = delete;
    SingleTon &operator= (const SingleTon &) = delete;
public:
    static SingleTon &getInstance() {
        static SingleTon single;
        return single;
    }
};




class SingleAutoSafe;
class SafeDeletor
{
public:
	void operator()(SingleAutoSafe* sf)
	{
		std::cout << "this is safe deleter operator()" << std::endl;
		delete sf;
	}
};

//可能不安全，存在reorder问题
class SingleAutoSafe
{
private:
	SingleAutoSafe() {}
	~SingleAutoSafe()
	{
		std::cout << "this is single auto safe deletor" << std::endl;
	}
	SingleAutoSafe(const SingleAutoSafe&) = delete;
	SingleAutoSafe& operator=(const SingleAutoSafe&) = delete;
	//定义友元类，通过友元类调用该类析构函数
	friend class SafeDeletor;
public:
	static std::shared_ptr<SingleAutoSafe> GetInst()
	{
		
		if (single != nullptr)
		{
			return single;
		}
		s_mutex.lock();
		
		if (single != nullptr)
		{
			s_mutex.unlock();
			return single;
		}
		
		single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDeletor());
		//也可以指定删除函数
		// single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, SafeDelFunc);
		s_mutex.unlock();
		return single;
	}
private:
	static std::shared_ptr<SingleAutoSafe> single;
	static std::mutex s_mutex;
};

std::shared_ptr<SingleAutoSafe> SingleAutoSafe::single = nullptr;
std::mutex SingleAutoSafe::s_mutex;


//利用call_once实现单例
class SingletonOnce {
private:
	SingletonOnce() = default;
	SingletonOnce(const SingletonOnce&) = delete;
	SingletonOnce& operator = (const SingletonOnce& st) = delete;
	static std::shared_ptr<SingletonOnce> _instance;

public :
	static std::shared_ptr<SingletonOnce> GetInstance() {
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]() {
			_instance = std::shared_ptr<SingletonOnce>(new SingletonOnce);
			});

		return _instance;
	}

	void PrintAddress() {
		std::cout << _instance.get() << std::endl;
	}

	~SingletonOnce() {
		std::cout << "this is singleton destruct" << std::endl;
	}
};

std::shared_ptr<SingletonOnce> SingletonOnce::_instance = nullptr;



template <typename T>
class Singleton {
protected:
	Singleton() = default;
	Singleton(const Singleton<T>&) = delete;
	Singleton& operator=(const Singleton<T>& st) = delete;
	static std::shared_ptr<T> _instance;
public:
	static std::shared_ptr<T> GetInstance() {
		static std::once_flag s_flag;
		std::call_once(s_flag, [&]() {
			_instance = std::shared_ptr<T>(new T);
			});
		return _instance;
	}
	void PrintAddress() {
		std::cout << _instance.get() << std::endl;
	}
	~Singleton() {
		std::cout << "this is singleton destruct" << std::endl;
	}
};
template <typename T>
std::shared_ptr<T> Singleton<T>::_instance = nullptr;

//想使用单例类，可以继承上面的模板，我们在网络编程中逻辑单例类用的就是这种方式
class LogicSystem :public Singleton<LogicSystem>
{
	friend class Singleton<LogicSystem>;
public:
	~LogicSystem(){}
private:
	LogicSystem(){}
};









template <typename T>
class SafeDeletor_T
{
public:
    void operator()(T *st)
    {
        delete st;
    }
};

template <typename T>
class Single_T
{
protected:
    Single_T() = default;
    Single_T(const Single_T<T> &st) = delete;
    Single_T &operator=(const Single_T<T> &st) = delete;
    ~Single_T()
    {
        std::cout << "this is auto safe template destruct" << std::endl;
    }
    void Delete(){
        delete this;
    }
public:
    static std::shared_ptr<T> GetInst()
    {
        if (single != nullptr)
        {
            return single;
        }

        s_mutex.lock();
        if (single != nullptr)
        {
            s_mutex.unlock();
            return single;
        }
        //额外指定删除器
        single = std::shared_ptr<T>(new T, SafeDeletor_T<T>());
        //也可以指定删除函数
        //single = std::shared_ptr<SingleAutoSafe>(new SingleAutoSafe, std::bind(Single_T::Delete, this));
        s_mutex.unlock();
        return single;
    }

private:
    static std::shared_ptr<T> single;
    static std::mutex s_mutex;
};

//模板类的static成员要放在h文件里初始化
template <typename T>
std::shared_ptr<T> Single_T<T>::single = nullptr;

template <typename T>
std::mutex Single_T<T>::s_mutex;

//通过继承方式实现网络模块单例
class SingleNet : public Single_T<SingleNet>
{
private:
    SingleNet() = default;
    SingleNet(const SingleNet &) = delete;
    SingleNet &operator=(const SingleNet &) = delete;
    ~SingleNet() = default;
    friend class SafeDeletor_T<SingleNet>;
    friend class Single_T<SingleNet>;
};


std::once_flag flag;
void thead_work1(std::string str) {
    std::cout << "str is " << str << std::endl;
}

void dosmt() {
    std::call_once(flag, [](){
        std::cout << "do something call once." << std::endl;
    });

    std::cout << "thread id is " << std::this_thread::get_id() << std::endl;

}


#endif