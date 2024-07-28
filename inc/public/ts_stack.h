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
        //改动栈容器前设置返回值
            std::shared_ptr<T> const res(std::make_shared<T>(data.top()));    
            data.pop();
        return res;
    }

    void pop(T& value)
    {
        std::lock_guard<std::mutex> lock(m);
        // pop空栈返回空栈异常
        if (data.empty()) throw empty_stack();
        value = data.top();
        data.pop();
    }
    
    bool empty() const
    {
        std::lock_guard<std::mutex> lock(m);
        return data.empty();
    }
};

#endif