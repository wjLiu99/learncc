#ifndef _M_VECTOR_H_
#define _M_VECTOR_H_

#include <memory>

//为什么要将分配内存和构造对象分开，我的理解是更好的对内存进行管理，可以将内存管理和构造对象分开
//如果每次插入对象都要分配内存的话会大大降低效率，因为这要重新分配内存，再把之前的数据拷贝过来
//先分配内存的话可以分配多一些的空间，以便下次构造对象时使用，直接使用定位new
template<typename T>
class myvector {
public:
    //构造函数
    myvector () : element(nullptr),
                first_free(nullptr),
                cap(nullptr)
    {}
    ~myvector();
    //拷贝构造
    myvector (const myvector&);
    //移动拷贝构造
    myvector (myvector &&src) noexcept: element(src.element),
                                        first_free(src.first_free),
                                        cap(src.cap)
    {
        src.element = src.first_free = src.cap = nullptr;
    }
    //赋值运算符
    myvector &operator= (const myvector &);
    size_t size() const { return first_free - element; }
    size_t capacity () const { return cap - element; }

    //返回第一个元素
    T *begin() const { return element; }
    //返回第一个空闲位置
    T *end() const {return first_free; }

    void push_back (const T &);
    void pop_back (T &);

    template<class... Args>
    void emplace_back (Args &&...args);


private:
    T *element;     //数组首元素指针
    T *first_free;  //第一个空闲元素指针
    T *cap;         //尾后指针
    void free();
    static std::allocator<T> alloc;

    void check_alloc () {   //容量不足重新开辟空间
        if (size() == capacity()) {
            reallocate();
        }
    }

    void reallocate();      //重新开辟空间
    std::pair<T*, T*> alloc_n_copy(const T*, const T*);
};

template <typename T>
std::allocator<T> myvector<T>::alloc;

template <typename T>
void myvector<T>::free()
{
    //先判断elements是否为空
    if (element == nullptr)
    {
        return;
    }

    auto dest = element;
    //遍历析构每一个对象
    for (size_t i = 0; i < size(); i++)
    {
        // destroy 会析构每一个元素
        alloc.destroy(dest++);
    }
    //再整体回收内存
    alloc.deallocate(element, cap - element);
    element = nullptr;
    cap = nullptr;
    first_free = nullptr;
}


//区间拷贝
template <typename T> 
std::pair<T*, T*> myvector<T>::alloc_n_copy(const T*b, const T*e) {
    auto newdata = alloc.allocate(e - b);                       //开辟空间，返回空间首地址
    auto first_free = std::uninitialized_copy(b, e, newdata);   //拷贝元素，返回第一个空闲空间
    return {newdata, first_free};
}

//拷贝构造
template <class T>
myvector<T>::myvector(const myvector &v) {
    auto rsp = alloc_n_copy(v.begin(), v.end());
    element = rsp.first;
    first_free = rsp.second;
    cap = rsp.second;
}


template<typename T>
myvector<T> &myvector<T>::operator= (const myvector &v) {
    //一定要判断自赋值
    if (this == &v) {
        return *this;
    }

    auto rsp = alloc_n_copy(v.begin(), v.end());
    element = rsp.first;
    first_free = rsp.second;
    cap = rsp.sencond;
}

template<typename T>
myvector<T>::~myvector() {
    //判断是否为空
    if (nullptr == element) {
        return;
    }

    auto dest = element;
    for (size_t i = 0; i < size(); i++) {
        alloc.destroy(dest++);  //先析构
    }

    alloc.deallocate(element, cap - element); //回收总容量，包括无效元素
    element = cap = first_free = nullptr;

}


template<typename T>
void myvector<T>::reallocate() {
    T *newdata = nullptr;
    //如果数组为空
    if (nullptr == element || nullptr == cap || nullptr == first_free) {
        newdata = alloc.allocate(1);
        first_free = element = newdata;
        cap = newdata + 1;
        return;
    }
    //数组不为空
    newdata = alloc.allocate(size() * 2);
    
    auto dest = newdata;        //新开辟的空间
    auto src = element;         //源数组首地址

    for (size_t i = 0; i < size(); ++i) {
        alloc.construct(dest++, std::move(*src++)); //直接使用转移语义，转移元素生命周期减少数据重新构造和源数据析构的开销
    }

    free();     //一定要删除源数据
    element = newdata;
    first_free = dest;
    cap = newdata + size() * 2;
}

template<typename T>
void myvector<T>::push_back (const T &t) {
    check_alloc();
    //const不会被move？只会copy？
    alloc.construct(first_free++, t); //这里不要用move操作，外部传的参数在这里是引用，使用move后资源转移，外部的实参也失效
}

template<typename T>
void myvector<T>::pop_back (T &t) {
    //判断容器是否为空
    if (nullptr == first_free) {
        return;
    }

    if (1 == size()) {
        t = *element;
        alloc.destroy(element); //析构
        first_free = element = nullptr;
        return;

    }

    t = *(--first_free);
    alloc.destroy(first_free);
}

//模板类型可变长，参数个数可变长，多对多关系
//std::forward<Args>args... 扩展模板参数包和函数参数包
//有模板中间传递一层的函数调用一定要完美转发
template<class T>
template <class... Args>
void myvector<T>::emplace_back (Args &&...args) {
    check_alloc();
    alloc.construct(first_free++, std::forward<Args>(args)...);
}
#endif