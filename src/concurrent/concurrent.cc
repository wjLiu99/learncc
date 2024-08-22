#include "concurrent.h"
#include <thread>
#include <iostream>
 
void print_str (int i, std::string s) {

}

void func1 (int &a) {

}
void danger_oops(int som_param) {
    char buffer[1024];
    sprintf(buffer, "%i", som_param);
    //在线程内部将char const* 转化为std::string
    //指针常量  char * const  指针本身不能变
    //常量指针  const char * 指向的内容不能变
    //将参数传递给thread并不会立刻发生隐式类型转换，会把参数传到thread内部的成员变量里，等线程开始运行的时候才会进行类型转换
    //当出作用域时，buf局部变量析构，线程再启动使用无效的局部变量就会出问题，可以将buf转换成右值传递给thread就不会出问题了，直接使用显示类型转换  
    //调用thread的构造函数会把传递的参数存到线程对象的成员变量内
    //传递给线程的参数都会拷贝成一个副本的方式传给调用函数
    int a = 3;
    //线程类内部会将参数转换成右值传递给回调函数，如果回调函数接收左值引用就会报错，要用ref保存参数的类型再传递
    std::thread t1(func1, std::ref(a));
    std::thread t(print_str, 3, buffer);
    t.detach();
    std::cout << "danger oops finished " << std::endl;
}

/*
    构造线程的时候传递参数会用decay去掉引用类型变成最基础的类型保存在tuple里，线程运行的时候全部move转成右值传入给回调函数
*/

/*
/// remove_reference
  template<typename _Tp>
    struct remove_reference
    { typedef _Tp   type; };

  template<typename _Tp>
    struct remove_reference<_Tp&>
    { typedef _Tp   type; };

  template<typename _Tp>
    struct remove_reference<_Tp&&>
    { typedef _Tp   type; };


template<typename _Tp>
    constexpr _Tp&&
    forward(typename std::remove_reference<_Tp>::type& __t) noexcept
    { return static_cast<_Tp&&>(__t); }

  template<typename _Tp>
    constexpr _Tp&&
    forward(typename std::remove_reference<_Tp>::type&& __t) noexcept
    {
      static_assert(!std::is_lvalue_reference<_Tp>::value, "template argument"
		    " substituting _Tp is an lvalue reference type");
      return static_cast<_Tp&&>(__t);
    }
  template<typename _Tp>
    constexpr typename std::remove_reference<_Tp>::type&&
    move(_Tp&& __t) noexcept
    { return static_cast<typename std::remove_reference<_Tp>::type&&>(__t); }

*/