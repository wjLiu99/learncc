#include <iostream>


class test {

public:
    test (int a) {
        std::cout << "test ()" << std::endl;
    }

    test (const test &) {
        std::cout << "test (&)" << std::endl;
    }

    test (test &&) {
        std::cout << "test (&&)" << std::endl;
    }

    ~test () {
        std::cout << "~test" << std::endl;
    }
};

template <typename T>
struct myalloctor {
    T *allocate (size_t size) {
        return (T *)malloc(sizeof(T) * size);
    }
    template <typename... Args>
    void construct (T *ptr, Args&& ...arg) {
        new (ptr)T(std::forward<Args>(arg)...);
    }
};



template <typename T, typename alloc = myalloctor<T>>
class vector {
public:
    vector () : vec_(nullptr), size_(0), idx_(0) {}
    void reserve (size_t size) {
        vec_ = allocator_.allocate(size);
        size_ = size;
    }

    template <typename Args>
    void push_back(Args&& arg) {
        allocator_.construct(vec_ + idx_, std::forward<Args>(arg));
    }
    //可变参模板，完美转发
    template <typename... Args>
    void emplace_back(Args&& ...arg) {
        allocator_.construct(vec_ + idx_, std::forward<Args>(arg)...);
    }

private:
    T *vec_;
    int size_;
    int idx_;
    alloc allocator_;
};

void vector_test () {
    vector<test> v;
    v.reserve(10);
    test t1(3);

    std::cout << "============" << std::endl;
    v.push_back(test(3));
    v.emplace_back(test(3));
    std::cout << "============" << std::endl;
    v.emplace_back(3);
   
    std::cout << "============" << std::endl;
}
