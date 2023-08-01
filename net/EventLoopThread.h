//
// Created by ftion on 2023/6/8.
//

#ifndef MUDUO_NET_EVENTLOOPTHREAD_H
#define MUDUO_NET_EVENTLOOPTHREAD_H

#include "../base/Mutex.h"
#include "../base/Condition.h"
#include "../base/Thread.h"

namespace muduo{
    namespace net{
        class EventLoop;

        class EventLoopThread : noncopyable{
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;

            EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(),
                            const string& name = string());
            ~EventLoopThread();
            EventLoop* startLoop();

        private:
            void threadFunc();

            EventLoop* loop_ GUARDED_BY(mutex_);  // 指向当前线程创建的循环Loop
            bool exiting_;          // 退出位线程标志
            Thread thread_;         // 需要运行的thread--->Func
            MutexLock mutex_;       // 锁
            Condition cond_ GUARDED_BY(mutex_);  // 初始化的条件变量
            ThreadInitCallback callback_;        // 线程的初始化回调函数
        };


    }
}
#endif //MUDUO_NET_EVENTLOOPTHREAD_H
