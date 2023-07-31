//
// Created by fight on 2023/5/21.
//

#ifndef MUDUO_BASE_COUNTDOWNLATCH_H
#define MUDUO_BASE_COUNTDOWNLATCH_H

#include "noncopyable.h"
#include "Mutex.h"
#include "Condition.h"

/*
 * 直译为 倒计时闩，当倒计时结束才执行下一部操作。
 * 通常应用在主线程阻塞等待其他多个线程准备就绪后继续执行。
 * 具体实现为：设定一个需要准备就绪事件的数目变量，该变量被互斥锁保护，
 * 在其他多个线程中被修改（各线程准备就绪后，数目变量进行减一），
 * 主线程阻塞等待，一旦数目变量减到值为0，条件变量通知唤醒主线程。
 */

namespace muduo{
    class CountDownLatch : muduo::noncopyable{
    public:

        explicit CountDownLatch(int count);

        void wait();

        void countDown();

        int getCount() const;

    private:
        mutable MutexLock mutex_;
        Condition condition_ GUARDED_BY(mutex_);
        int count_ GUARDED_BY(mutex_);
    };
}

#endif //MUDUO_BASE_COUNTDOWNLATCH_H
