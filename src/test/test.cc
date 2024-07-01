#include "test.h"
#include "threadpool.h"
#include "asynclog.h"
#include <iostream>
#include <thread>
#include <sstream>
#include <vector>
#include <numeric> 
#include <cstring>
#include "mvector.h"
// #include "singleton.h"
#include <algorithm>



void myvec_test () {
    auto v1 = myvector<std::string>();
    v1.push_back("hello liu");
    myvector<std::string> v2(v1);
    v2.push_back("hello wen");
    myvector<std::string> v3 = v1;

    std::for_each(v2.begin(), v2.end(), [](auto data) {
        std::cout << data << " ";
    });
}
int main()
{   
    myvec_test();
    return 0;
}