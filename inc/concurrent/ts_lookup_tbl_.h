#ifndef _TS_LOOKUP_TBL_
#define _TS_LOOKUP_TBL_

#include <shared_mutex>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <mutex>
#include <list>
#include <map>
#include <iterator>

//thread safe lookup table
template <typename K, typename V, typename Hash = std::hash<K>>
class ts_lookup_tbl {
private:
    class bucket_t {
        friend class ts_lookup_tbl;
    private:
        typedef std::pair<K, V> buctet_value;           //存储键值数据类型
        typedef std::list<buctet_value> bucket_data;    //链表存储

        typedef typename bucket_data::iterator bucket_iterator; //链表迭代器，要显式指明iterator是一个类型不是变量
        bucket_data data;
        mutable std::shared_mutex mutex;
    }
};


/*

    对于锁的精度控制不够，对于链表的操作都是用一个共享锁实现，有改进的空间
*/
#endif