#ifndef _TEMPLATE_H_
#define _TEMPLATE_H_

#include <memory>
#include <iostream>

template<typename T>
void f1 (T &&t) {
    //当模版函数的参数是一个T类型的右值引用时，传递一个实参是左值时T被推断为T&类型，传递一个右值时推断为T类型
    //传入一个左值时 T被推断为T&类型，则t为T& &&类型，根据引用折叠规则t为T&类型，就是T的左值引用类型
    //传入一个右值时 T被推断为T类型， 则t为T &&类型，则t是一个T的右值引用类型
    //折叠规则只在模版中存在 规则：
    //T&& T& &&被折叠为T&
    //T&& &&折叠为T&&
    
}


//std::move实现
//当传入int类型的左值时，T被推断为int& 类型，remove_reference<int&>的type成员为 int类型，返回值是int&&类型，就是int的右值引用类型
//形参t为int& &&类型，被折叠为int&类型，就是将int的左值引用类型转换为int的右值引用类型。通过static_cast进行转换
//当传入int类型的右值时， T被推断为int&&类型，remove_reference<int&&>::type为 int类型，返回值是int&&类型，就是int的右值引用类型
//形参t为int&& &&类型，折叠为int&& 将int的右值引用转换为右值引用等于什么都没做，返回值是int的右值引用,形参的类型是右值引用，但是t本身已经是左值
template<typename T>
typename std::remove_reference<T>::type && my_move(T&& t){
    return static_cast<typename std::remove_reference<T>::type &&>(t);
}


void move_test () {
    //remove_reference<int &>::type 就是一个int
    //remove_reference<string &>::type 类型是string
    

}


template<typename F, typename T1, typename T2>
void addtp (F f, T1 &&t1, T2 &&t2) {
    //不管外部传参是左值还是右值，函数内部形参t1, t2都是左值，传左值时左值引用，传右值时右值引用
    //再将参数传给f时，如果f的参数为右值引用则报错，因为左值不能传给右值引用，这时候就需要完美转发
    f(t2, t1);
    f(std::forward<T2>(t2), std::forward<T1>(t1));
}

//可变参数模板
template<typename T>
std::ostream &print(std::ostream &os, const T &t) {
    return os << t;     //输出最后一个元素，递归出口
}
//const 引用类型也是万能类型，参数后加...表示参数展开，类型...表示可变参数类型，模板参数包
template<typename T, typename... Args>
std::ostream &print (std::ostream os, const T&t, Args &...arg) {
    os << t << ",";
    return print(os, arg...); //参数展开
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

#endif