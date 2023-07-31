//
// Created by ftion on 2023/5/29.
//

#ifndef MUDUO_BASE_ASYNCLOGGING_H
#define MUDUO_BASE_ASYNCLOGGING_H

#include "BlockingQueue.h"
#include "BoundedBlockingQueue.h"
#include "CountDownlatch.h"
#include "Mutex.h"
#include "Thread.h"
#include "LogStream.h"

#include <atomic>
#include <vector>

namespace muduo{
    /*
     * 多个线程共有一个前端，通过后端写入磁盘文件
     * 异步日志是必须的，所以需要一个缓冲区，
     * 在这里我们使用的是多缓冲技术，基本思路是准备多块Buffer，
     * 前端负责向Buffer中填数据，后端负责将Buffer中数据取出来写入文件，
     * 这种实现的好处在于在新建日志消息的时候不必等待磁盘IO操作，前端写的时候也不会阻塞。
     */
    class AsyncLogging : noncopyable{
    public:
        AsyncLogging(const string& basename,
                     off_t rollSize,
                     int flushInterval = 3);

        ~AsyncLogging()
        {
            if (running_)
            {
                stop();
            }
        }

        void append(const char* logline, int len);

        void start()
        {
            running_ = true;
            thread_.start();
            latch_.wait();
        }

        void stop() NO_THREAD_SAFETY_ANALYSIS
        {
            running_ = false;
            cond_.notify();
            thread_.join();
        }


    private:
        void threadFunc();

        // 缓冲
        typedef muduo::detail::FixedBuffer<muduo::detail::kLargeBuffer> Buffer;
        typedef std::vector<std::unique_ptr<Buffer>> BufferVector;
        typedef BufferVector::value_type BufferPtr;

        const int flushInterval_;
        std::atomic<bool> running_;
        const string basename_;
        const off_t rollSize_;
        muduo::Thread thread_;
        muduo::CountDownLatch latch_;
        muduo::MutexLock mutex_;

        muduo::Condition cond_ GUARDED_BY(mutex_);
        // 当前 和 下一个缓冲区
        BufferPtr currentBuffer_ GUARDED_BY(mutex_);
        BufferPtr nextBuffer_ GUARDED_BY(mutex_);
        // 待写入文件的已填满缓冲，供后端写入
        BufferVector buffers_ GUARDED_BY(mutex_);

    };
}


#endif //MUDUO_BASE_ASYNCLOGGING_H
