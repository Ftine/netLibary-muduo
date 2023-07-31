//
// Created by fight on 2023/5/22.
//

#ifndef MUDUO_BASE_BLOCKINGQUEUE_H
#define MUDUO_BASE_BLOCKINGQUEUE_H

#include "Condition.h"
#include "Mutex.h"

#include <deque>
#include <assert.h>

/*
 * 线程安全、阻塞的队列。队列使用std::deque双端队列，可以在队列首、尾插入或删除元素。
 * BlockingQueue类使用Mutex在这里插入代码片保护成员变量；
 * 为提高效率（出队take()操作比入队put()操作要频繁），使用Condition变量，
 * 当读出数据时，发现这个队列是空的，就把这个线程挂起到条件变量上，当写入数据的时候就唤起所有正在等待条件变量的线程。
 */

namespace muduo{
    template<typename T>
    class BlockingQueue : noncopyable
    {
    public:
        BlockingQueue() : mutex_(),  notEmpty_(mutex_), queue_()
        {}

        void put(const T& x)
        {
            MutexLockGuard lock(mutex_);   //往队列中加入元素时，需要加锁保护
            queue_.push_back(x);
            notEmpty_.notify();  // 加入元素后，通知需要读取线程
        }

        void put(T&& x)
        {
            MutexLockGuard lock(mutex_);    //往队列中加入元素时，需要加锁保护
            queue_.push_back(std::move(x));
            notEmpty_.notify();   // 加入元素后，通知需要读取线程
        }

        T take()
        {
            MutexLockGuard lock(mutex_); //加锁
            while (queue_.empty())
            {
                notEmpty_.wait();  // 如果队列为空，线程被挂起，等待条件变量来唤起
            }
            assert(!queue_.empty());
            T front(std::move(queue_.front()));   // 移动返回队列第一个元素
            queue_.pop_front();  // 弹出队列的第一个元素
            return front;
        }

        size_t size() const
        {
            MutexLockGuard lock(mutex_); // 加锁
            return queue_.size();
        }

    private:
        mutable MutexLock mutex_;
        Condition         notEmpty_ GUARDED_BY(mutex_);
        std::deque<T>     queue_ GUARDED_BY(mutex_);
    };

}

#endif //MUDUO_BASE_BLOCKINGQUEUE_H
