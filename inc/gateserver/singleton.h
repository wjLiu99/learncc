#ifndef _SINGLETON_H
#define _SINGLETON_H
#include "comm.h"

//单例模版类
template <typename T>
class Singleton {
protected:
    Singleton() = default; //声明为受保护的是让子类可以调用基类的构造函数
    Singleton (const Singleton<T> &) = delete;
    Singleton & operator= (const Singleton<T> &) = delete;
    static std::shared_ptr<T> instance_;


public:
    static std::shared_ptr<T> get_instance() {
        std::once_flag flag;
        std::call_once(flag, [](){
            instance_ = std::shared_ptr<T>(new T); //为什么用new，不用make_shared

        });
        return instance_;
    };

    ~Singleton() {
        std::cout << "singleton desturct" << std::endl;
    }

    void PirntAddr() {
        std::cout << instance_.get() << std::endl;
    }

};

template <typename T>
std::shared_ptr<T> Singleton<T>::instance_ = nullptr;
#endif // SINGLETON_H
