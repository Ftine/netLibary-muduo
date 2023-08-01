//
// Created by ftion on 2023/5/31.
//

#ifndef MUDUO_NET_EVENTLOOP_H
#define MUDUO_NET_EVENTLOOP_H

#include <atomic>
#include <functional>
#include <vector>
#include <boost/any.hpp>

#include "../base/Mutex.h"
#include "../base/CurrentThread.h"
#include "../base/Timestamp.h"
#include "Callbacks.h"
#include "TimerId.h"

namespace muduo{
    namespace net{
        class Channel;
        class Poller;
        class TimerQueue;

        class EventLoop : noncopyable{
        public:
            typedef std::function<void()> Functor;

            EventLoop();
            ~EventLoop();

            void loop();
            void quit();


            // 它的主要作用是检查当前线程是否为 IO 线程，如果不是则会触发断言错误。
            void assertInLoopThread(){
                if(!isInLoopThread()){
                    abortNotInLoopThread();
                }
            }

            bool isInLoopThread() const { return threadId_ == CurrentThread::tid(); }

            static EventLoop* getEventLoopOfCurrentThread();



            //在循环线程中立即运行回调。
            //它唤醒循环，并运行 cb。
            //如果在同一个循环线程中，cb 在函数内运行。
            //可以安全地从其他线程调用。

            /*
             * Muduo网络库中的void runInLoop(Functor cb)函数是一个事件循环机制中的函数，
             * 用于在I/O线程中执行用户回调函数。它的作用是将用户回调函数添加到I/O线程的任务队列中，等待I/O线程在下一个事件循环中执行。
             *
             * 具体来说，当用户在某个线程中需要执行一个需要在I/O线程中执行的操作时，可
             * 以调用runInLoop函数，并将需要执行的操作封装为一个Functor对象（即一个可调用对象），
             * 作为参数传递给runInLoop函数。runInLoop函数会将这个Functor对象添加到I/O线程的任务队列中，等待I/O线程在下一个事件循环中执行。
             */
            void runInLoop(Functor cb);

            /*
             * 在循环线程中排队回调。
             * 在完成池化后运行。
             */
            void queueInLoop(Functor cb);


            //在指定时间戳time处添加一个定时器，并在定时器超时时执行回调函数cb
            TimerId runAt(Timestamp time, TimerCallback cb);
            //在指定时间延迟后添加一个定时器，并在定时器超时时执行回调函数cb
            TimerId runAfter(double delay, TimerCallback cb);
            //在指定时间区间后添加一个定时器，并在定时器超时时执行回调函数cb
            TimerId runEvery(double interval, TimerCallback cb);
            //关闭计时器
            void cancel(TimerId timerId);


            // 内部使用
            void wakeup() const;
            void updateChannel(Channel* channel);
            void removeChannel(Channel* channel);
            bool hasChannel(Channel* channel);

        private:
            void abortNotInLoopThread();
            void handleRead() const; // waked up;
            // 执行待执行的函数事件
            void doPendingFunctors();


            typedef std::vector<Channel*> ChannelList;

            bool looping_;              // 是否在循环中
            std::atomic<bool> quit_;    // 是否退出 终止了
            bool eventHandling_;        // 是否正在处理I/O事件
            bool callingPendingFunctors_;   // 是否处理待处理的函数
            int64_t iteration_;         // 处理I/O的次数
            const pid_t threadId_;      // 当前thread id

            Timestamp pollReturnTime_;                  //

            std::unique_ptr<Poller> poller_;            // Poller对象(监听socketfd封装的Channels)
            std::unique_ptr<TimerQueue> timerQueue_;    // TimerQueue对象(timerfd封装的channel)

            int wakeupFd_;              // eventfd描述符，用于唤醒阻塞的poll
            std::unique_ptr<Channel> wakeupChannel_;
            boost::any context_;

            // 暂存变量
            ChannelList activeChannels_;
            Channel* currentActiveChannel_;

            mutable MutexLock mutex_;

            //待执行的函数队列
            std::vector<Functor> pendingFunctors_ GUARDED_BY(mutex_);


            void printActiveChannels() const;
        };
    } // net
} // muduo

#endif //MUDUO_NET_EVENTLOOP_H
