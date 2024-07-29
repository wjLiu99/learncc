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

//线程安全的哈希查找表
template <typename k, typename v, typename Hash = std::hash<k>>
class ts_hash_tbl {
private:
    class bucket_t {
        friend class ts_hash_tbl;
    private:
        typedef std::pair<k, v> data;           //存储键值数据类型
        typedef std::list<data> data_list;    //链表存储

        typedef typename data_list::iterator b_it; //链表迭代器，要显式指明iterator是一个类型不是变量
        data_list list_;
        mutable std::shared_mutex mtx_;

        b_it find_entry (const k &key) {
            return std::find_if (list_.begin(), list_.end(), [&](const data &other){
                return other.first == key;
            });
        }
    public:

        // //查找key对应的value，没找到返回默认值
        v value_for (const k &key, const v &default_value) {
            std::shared_lock<std::shared_mutex> lolkck(mtx_);
            const b_it entry = find_entry(key);
            return entry == list_.end() ? default_value : entry->second;
        }

        //添加key，找到更新，没找到添加
        template<typename Ty>
        void add (const k &key, Ty &&value) {
            std::unique_lock<std::shared_mutex> lk(mtx_);
            const b_it entry = find_entry(key);
            if (entry == list_.end()) {
                list_.emplace_back(key, std::forward<Ty>(value));
                return;
            }

            entry->second = std::forward<Ty>(value);
        }

        //删除key
        void remove (const k &key) {
            std::unique_lock<std::shared_mutex> lk(mtx_);
            const b_it entry = find_entry(key);
            if (entry != list_.end()) {
                list_.erase(entry);
            }
        }
    };

    //外部用vector存储桶
    std::vector<std::unique_ptr<bucket_t>> buckets_;

    Hash hasher_;
    //根据key生成数字，并对桶的大小取余得到下标，根据下标返回对应的桶智能指针
	bucket_t &get_bucket(const k& key) const
	{
		std::size_t const bucket_index = hasher_(key) % buckets_.size();
		return *buckets_[bucket_index]; 
    }
public:
    //初始化的是空指针，需要创建桶然后reset
    ts_hash_tbl (uint32_t bucket_num = 13, const Hash &hasher = Hash()) : buckets_(bucket_num), hasher_(hasher) {
        for (uint32_t i = 0; i < bucket_num; ++i) {
            buckets_[i].reset(new bucket_t);
        }
    }

    ts_hash_tbl (const ts_hash_tbl &) = delete;
    ts_hash_tbl& operator=(const ts_hash_tbl &) = delete;
    //查找key，没有返回默认值
    v find (const k &key, const v &default_value = v()) {
        return get_bucket(key).value_for(key, default_value);
    }
    //插入key
    template<typename Ty>
    void insert (const k &key, Ty &&arg) {
        return get_bucket(key).add(key, std::forward<Ty>(arg));
    }
    //删除key
    void remove (const k &key) {
        return get_bucket(key).remove(key);
    }

    //将hash查找表打包成map
    std::map<k, v> get_map() 
	{
        //将所有的bucket锁住进行拷贝
		std::vector<std::unique_lock<std::shared_mutex>> locks;
		for (unsigned i = 0; i < buckets_.size(); ++i) {
            //uniquelock只有右值拷贝
			locks.push_back(
				std::unique_lock<std::shared_mutex>(buckets_[i]->mtx_));
		}
		std::map<k, v> res;
		for (unsigned i = 0; i < buckets_.size(); ++i) {
			//需用typename告诉编译器bucket_type::bucket_iterator是一个类型，以后再实例化
			//当然此处可简写成auto it = buckets[i]->data.begin();
			typename bucket_t::b_it it = buckets_[i]->data.begin();
			for (; it != buckets_[i]->list_.end(); ++it)
			{
				res.insert(*it);
			}
		}
		return res;
	}

};


/*

    对于锁的精度控制不够，对于链表的操作都是用一个共享锁实现，有改进的空间
*/
#endif