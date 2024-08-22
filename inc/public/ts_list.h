#ifndef _TS_LIST_H_
#define _TS_LIST_H_
#include <mutex>
#include <memory>

//线程安全的链表
template<typename T>
class threadsafe_list
{
    struct node
    {
        std::mutex mtx_;
        std::shared_ptr<T> data_;
        std::unique_ptr<node> next_;
        node() : next_()
        {}
        node(const T &value) :
            data_(std::make_shared<T>(value))
        {}
    };
    node head_;                 //头结点
    node *last_node_ptr_;       //尾节点指针
    std::mutex last_ptr_mtx_;   //互斥访问尾节点指针
public:
    threadsafe_list()  {
        last_node_ptr_ =  &head_;
    }

    ~threadsafe_list() { remove_if([] (const node &){ return true;});}
 
    threadsafe_list(const threadsafe_list &other) = delete;
    threadsafe_list& operator=(const threadsafe_list &other) = delete;
    bool empty () const {
        std::lock_guard<std::mutex> lk(last_ptr_mtx_);
        return last_node_ptr_ ==  &head_;
    }
    //满足谓词条件的节点删除
    template<typename predicate>
    void remove_if (predicate p) {
        node *cur = &head_;
        std::unique_lock<std::mutex> lk(head_.mtx_);
        while ( node * const next = cur->next_.get()) {
            std::unique_lock<std::mutex> next_lk(next->mtx_);
            //节点数据满足谓词删除
            if (p(*next->data_)) {
                std::unique_ptr<node> old = std::move(cur->next_);
                cur->next_ = std::move(next->next_);
                //如果删除的是最后一个节点，尾节点指针需要往前移动
                if (cur->next_ == nullptr) {
                    std::lock_guard<std::mutex> lk(last_ptr_mtx_);
                    last_node_ptr_ = cur;
                }
                next_lk.unlock();
                continue;
            }
            //不满足谓词遍历下一个节点
            lk.unlock();
            cur = next;
            lk = std::move(next_lk);
        }
    } 

    //满足谓词条件的第一个节点删除
    template<typename predicate>
    bool remove_first (predicate p) {
        node *cur = &head_;
        std::unique_lock<std::mutex> lk(head_.mtx_);
        while ( node * const next = cur->next_.get()) {
            std::unique_lock<std::mutex> next_lk(next->mtx_);
            //节点数据满足谓词删除
            if (p(*next->data_)) {
                std::unique_ptr<node> old = std::move(cur->next_);
                cur->next_ = std::move(next->next_);
                //如果删除的是最后一个节点，尾节点指针需要往前移动
                if (cur->next_ == nullptr) {
                    std::lock_guard<std::mutex> lk(last_ptr_mtx_);
                    last_node_ptr_ = cur;
                }
                next_lk.unlock();
                return true;
            }
            //不满足谓词遍历下一个节点
            lk.unlock();
            cur = next;
            lk = std::move(next_lk);
        }
        return false;
    } 

    /*
        多线程并发只有链表为空时头插和尾插才有资源竞争都会操作头结点和尾节点指针
        只要保证先锁住前一节点再将数据插入，再修改指针就不会产生线程安全问题

    */
    //头部插入节点
    void push_front (const T &value) {
        std::unique_ptr<node> new_node(new node(value));
        std::lock_guard<std::mutex> lk(head_.mtx_);
        new_node->next_ = std::move(head_.next_);
        head_.next_ = std::move(new_node);
        //插入第一个节点时需要更新尾节点指针
        if (head_.next_->next_ == nullptr) {
            std::lock_guard<std::mutex> lk(last_ptr_mtx_);
            last_node_ptr_ = head_.next_.get();
        }


    }
    //尾部插入节点
    void push_back (const T &value) {
        std::unique_ptr<node> new_node(new node(value));
        //锁住尾节点,可以优化保证删除也是先获取节点锁再获取尾指针可以只锁尾节点，不用尾节点指针锁了
        std::lock_guard<std::mutex> lk(last_node_ptr_->mtx_);
        //锁住尾节点指针
        std::lock_guard<std::mutex> p_lk(last_ptr_mtx_);
        last_node_ptr_->next_ = std::move(new_node);
        last_node_ptr_ = last_node_ptr_->next_.get();
    }

    //根据谓词查找节点
    template<typename predicate>
    std::shared_ptr<T> find_first_if (predicate p) {
        node *cur = &head_;
        std::unique_lock<std::mutex> lk(head_.mtx_);
        while (node * const next = cur->next_.get()) {
            std::unique_lock<std::mutex> n_lk(next->mtx_);
            if (p(*next->data_)) {
                return next->data_;
            }
            cur = next;
            lk = std::move(n_lk);
        }

        return nullptr;
    }

    //遍历所有节点
    template<typename func>
    void for_each (func fun) {
        node *cur = &head_;
        std::unique_lock<std::mutex> lk(head_.mtx_);
        while ( node * const next = cur->next_.get()) {
            std::unique_lock<std::mutex> nlk(next->mtx_);
            lk.unlock();
            fun(*next->data_);
            cur = next;
            lk = std::move(nlk);

        }
    }



};

#endif