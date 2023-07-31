//
// Created by ftion on 2023/5/19.
//

#ifndef MUDUO_BASE_THREAD_H
#define MUDUO_BASE_THREAD_H
#include "Types.h"
#include "CurrentThread.h"
#include "noncopyable.h"
#include "CountDownlatch.h"
#include "Atomic.h"

#include <functional>
#include <memory>
#include <pthread.h>

#include <functional>
#include <memory>
#include <pthread.h>

/*
 * （1）函数start()：内部使用pthread_create()创建线程，使用CountDownLatch来等到回调函数进入初始化成功；
 * （2）函数join()：阻塞地等待回调函数执行结束；
 * （3）函数started()：判断线程是否正在执行中；
 * （4）函数tid()：返回当前线程tid；
 * （5）静态函数numCreated()：当前已经创建的线程数量，用于线程默认名称；
 * （6）函数setDefaultName()：设置线程默认名称，“Thread_” + 当前创建线程数量。
 * 原文链接：https://blog.csdn.net/wanggao_1990/article/details/118695903
 */


namespace muduo{
    class Thread : noncopyable
    {
    public:
        //std::function<void ()>是C++11标准库中的一个通用函数封装器，
        // 用于封装任意可调用对象，例如函数、函数指针、成员函数、仿函数等。
        typedef std::function<void ()> ThreadFunc;

        explicit Thread(ThreadFunc, const string& name = string());
        // FIXME: make it movable in C++11
        ~Thread();

        void start();
        int join(); // return pthread_join()

        bool started() const { return started_; }
        // pthread_t pthreadId() const { return pthreadId_; }
        pid_t tid() const { return tid_; }
        const string& name() const { return name_; }

        static int numCreated() { return numCreated_.get(); }

    private:
        void setDefaultName();

        bool       started_;   // 线程是否开始
        bool       joined_;    // 线程是否可以join
        pthread_t  pthreadId_; // 线程id
        pid_t      tid_;       // 主线程线程tid ---- 一个进程pid_t
        ThreadFunc func_;      // 线程回调函数
        string     name_;      // 线程名字
        CountDownLatch latch_; // 用于等待线程函数执行完毕
        // CountDownLatch类，这个类主要成员是计数值和条件变量，
        // 当计数值为0，条件变量发出通知。既可以用于所有子线程等待主线程发起 “起跑” ；
        // 也可以用于主线程等待子线程初始化完毕才开始工作。

        static AtomicInt32 numCreated_; // 原子操作，当前已经创建线程的数量
    };

}
#endif //MUDUO_BASE_THREAD_H
