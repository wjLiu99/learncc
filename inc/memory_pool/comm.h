#ifndef _COMM_H_
#define _COMM_H_

#include <iostream>
#include <assert.h>
#include <thread>
#include <mutex>
#include <vector>
#include <unordered_map>
#include <sys/mman.h> 

typedef size_t page_id;
const size_t NLISTS = 240;   //管理自由链表数组的长度,根据对齐规则计算出来的

const size_t MAXBYTES = 64 * 1024; //ThreadCache最大可以一次分配多大的内存64K

const size_t PAGE_SHIFT = 12;// 一页是4096字节,2的12次方=4096

const size_t NPAGES = 129;   //PageCache的最大可以存放NPAGES-1页

static inline void *sys_alloc (size_t npages) {
    void *p = mmap(0, (npages) << PAGE_SHIFT,  PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (MAP_FAILED == p) {
        throw std::bad_alloc();
    }

    return p;
}   

static inline void sys_free (void *p, size_t npages) {
    int ret = munmap(p, (npages) << PAGE_SHIFT);
    if (0 != ret) {
        throw std::bad_alloc();
    }
}
// 下一个内存块地址
static inline void *&next_block (void *ptr) {
    return *((void **)ptr);
}

// 自由链表类
class free_list {
private:
    void *list_ = nullptr;
    size_t size_ = 0;
    size_t max_size_ = 1;

public:
    bool empty () {
        return nullptr == list_;
    }

    // 将一段内存空间插入自由链表
    void push_range (void *start, void *end, size_t n) {
        next_block(end) = list_;
        list_ = start;
        size_ += n;
    }

    void *pop () {
        void *obj = list_;
        list_ = next_block(obj);
        size_--;
        return obj;
    }

    void push (void *ptr) {
        next_block(ptr) = list_;
        list_ = ptr;
        size_++;
    }

    size_t max_size () const {
        return max_size_;
    }

    void set_maxsize (size_t num) {
        max_size_ = num;
    }

    size_t size () const {
        return size_;
    }

    void *clear () {
        size_ = 0;
        void *list = list_;
        list_ = nullptr;
        return list;
    }
};

struct span {
    page_id page_id_;
    size_t npages_;

    span *next_;
    span *prev_;

    void *block_list_;  // 内存块链表首地址
    size_t block_size_; // span中每块内存块大小
    size_t used_count_; // span 的引用计数，被thread cache申请使用了多少块

};

// span 循环双向链表
class span_list {
private:
    span *head_ = nullptr;

public:
    std::mutex mtx_;
    span_list () {
        head_ = new span;
        head_->next_ = head_->prev_ = head_;
    }

    span *begin () const {
        return head_->next_;
    }

    span *end () const {
        // return head_;
        return head_->prev_;
    }

    span *head () const {
        return head_;
    }

    bool empty () const {
        return head_ == head_->next_;
    }

    // 在cur前插入新节点
    void insert (span *cur, span *newspan) {
        assert(cur);
        span *pre = cur->prev_;
        pre->next_ = newspan;
        newspan->prev_ = pre;
        cur->prev_ = newspan;
        newspan->next_ = cur;
    }
    // 删除当前节点
    void erase (span *cur) {
        assert(cur != nullptr && cur != head_);
        span *pre = cur->prev_;
        span *next = cur->next_;

        pre->next_ = next;
        next->prev_ = pre;
        cur->next_ = cur->prev_ = nullptr;
    }

    void push_front (span *cur) {
        insert(begin(), cur);
    }

    span *pop_front () {
        span *front = begin();
        erase(front);
        return front;
    }

    void push_back (span *cur) {
        insert(head_, cur);
    }

    span *pop_back () {
        span *back = end();
        erase(back);
        return back;
    }


};


class size_class {

public:
    // 向上调整为align的倍数
    static inline size_t round_up (size_t size, size_t align) {
        return (size + align -1) & ~(align - 1);
    }
    //控制内碎片在12%左右的浪费
	//[1, 128]						8byte对齐		freelist[0,16)
	//[129, 1024]					16byte对齐		freelist[17, 72)
	//[1025, 8 * 1024]				64byte对齐		freelist[72, 128)
	//[8 * 1024 + 1, 64 * 1024]		512byte对齐		freelist[128, 240)
    static inline size_t round_up (size_t size) {
        assert(size <= MAXBYTES);

        if (size <= 128) {
            return round_up(size, 8);
        }
        if (size <= 1024) {
            return round_up(size, 16);
        }
        if (size <= 8 * 1024) {
            return round_up(size, 128);
        }
        if (size <= 64 * 1024) {
            return round_up(size, 512);
        }

        return -1;

    }
	//求出在该区间的第几个,align_shift是对齐字节的二进制位数
    static size_t index (size_t bytes, size_t align_shift) {
		// 给bytes加上对齐数减一也就是
		// 让其可以跨越到下一个自由链表的数组的元素中
		return ((bytes + (1 << align_shift) - 1) >> align_shift) - 1;
    }

    static inline size_t index (size_t bytes) {
        assert(bytes < MAXBYTES);

        static int group_tbl[] = { 16, 56, 56, 112 };

        if (bytes <= 128) {
            return index(bytes, 3);
        }

        if (bytes <= 1024) {
            return index(bytes - 128, 4) + group_tbl[0];
        }

        else if (bytes <= 8 * 1024) {
			return index(bytes - 1024, 7) + group_tbl[1] + group_tbl[0];
		}

		if (bytes <= 64 * 1024) {
			return index(bytes - (8 * 1024), 9) + group_tbl[2] + group_tbl[1] + group_tbl[0];
		}

		return -1;

    }

    // 计算thread cache从central cache一次获取多少个size大小的空间
    static size_t move_num (size_t size) {
        if (0 == size) {
            return 0;
        }

        int num = (int) (MAXBYTES / size);
        if (num < 2) {
            num = 2;
        }
        if (num > 512) {
            num = 512;
        }

        return num;
    }

    // 计算thread cache从central cache一次获取多少页
    static size_t move_page (size_t size) {
        size_t num = move_num(size);
        size_t npage = (num * size) >> PAGE_SHIFT;
        if (0 == npage) {
            return 1;
        }
        return npage;
    }
};
#endif