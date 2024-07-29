#ifndef _TS_STACK_H_
#define _TS_STACK_H_
#include <stdexcept> // 引入标准异常类

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

#endif