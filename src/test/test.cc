#include "test.h"
#include "threadpool.h"
#include "asynclog.h"
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <numeric> 
#include <cstring>

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
void func1 (int &a) {
    std::cout << "zuoz" << std::endl;
    a = 1;
}

void func1 (int &&aa) {
    std::cout << "youzi" << std::endl;
    aa = 12;
}

template <typename T>
void func2 (T &&a) {
    func1(std::forward<T>(a));
}

class AA {
public:
    int *data_ = nullptr;
    AA() = default;
    ~AA() {
        if (nullptr != data_) {
            delete data_;
            data_ = nullptr;
        }
    }
    void alloc() {
        data_ = new int;
        memset(data_, 0, sizeof(int));
    }
    AA (const AA& a) {
        std::cout << "调用拷贝构造函数" << std::endl;
        if (data_ == nullptr) {
            alloc();
        }
        memcpy(data_, a.data_, sizeof(int));
    }

    AA (AA &&a) {
        std::cout << "调用移动构造函数" << std::endl;
        if (nullptr != data_) {
            delete data_;
        }
        data_ = a.data_;
        a.data_ = nullptr;
    }

    AA &operator= (const AA&a) {
        std::cout << "调用赋值函数" <<std::endl;
        if (this == &a) return *this;
        if (data_ == nullptr) alloc();
        memcpy(data_, a.data_, sizeof(int));
        return *this;
    }

    AA &operator= (AA &&a) {
        std::cout << "调用移动赋值函数" << std::endl;
        if (&a == this) return *this;
        if (nullptr != data_) delete data_;
        data_ = a.data_;
        a.data_ = nullptr;
        return *this;
    }

};
AA fun () {
    AA aa;
    aa.alloc();
    *aa.data_ = 8;
    
    return aa;
} 
int main()
{   
    int a = 5;
    func2(a);
    func2(5);
    return 0;
}