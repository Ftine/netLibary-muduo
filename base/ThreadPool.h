//
// Created by fight on 2023/5/22.
//

#ifndef MUDUO_BASE_THREADPOOL_H
#define MUDUO_BASE_THREADPOOL_H

#include "Condition.h"
#include "Mutex.h"
#include "Thread.h"
#include "Types.h"

#include <deque>
#include <vector>

namespace muduo{
    class ThreadPool: noncopyable{
    public:
        typedef std::function<void ()> Task; // 执行函数

        explicit ThreadPool(const string& nameArg = string("ThreadPool"));
        ~ThreadPool();

        void setMaxQueueSize(int maxSize) { maxQueueSize_ = maxSize; }  // 设置任务数量上限
        void setThreadInitCallback(const Task& cb)  					  // 线程初始化后的操作
        { threadInitCallback_ = cb; }

        void start(int numThreads);	// 设置线程池中线程数量，并启动线程池
        void stop();  				// 关闭线程池

        const string& name() const { return name_; }
        size_t queueSize() const;
        void run(Task f); // 加入一个任务进入队列 AppendTask

    private:
        bool isFull() const REQUIRES(mutex_);
        void runInThread(); // 执行
        Task take(); // 获取一个任务

        mutable MutexLock mutex_;
        Condition notEmpty_ GUARDED_BY(mutex_);
        Condition notFull_ GUARDED_BY(mutex_);
        string name_;
        Task threadInitCallback_;  // 任务初始化回调
        std::vector<std::unique_ptr<muduo::Thread>> threads_; //线程pool
        std::deque<Task> queue_ GUARDED_BY(mutex_);  // 任务队列
        size_t maxQueueSize_;
        bool running_;

    };
}

#endif //MUDUO_BASE_THREADPOOL_H
