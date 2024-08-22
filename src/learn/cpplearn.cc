
// #include "learncpp.h"
extern void vector_test();
// extern void memleak_test();
#include <vector>
#include "myallocator.h"
int main () {
    
    std::vector<int, myallocator<int>> vec;
    vec.push_back(1);
    vec.push_back(1);
    vec.push_back(1);

    for (auto & i : vec) {
        std::cout << i << std::endl;
    }

    return 0;
}