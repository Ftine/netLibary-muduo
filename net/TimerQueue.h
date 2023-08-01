//
// Created by fight on 2023/5/30.
//

#ifndef MUDUO_NET_TIMERQUEUE_H
#define MUDUO_NET_TIMERQUEUE_H

#include <set>
#include <vector>

#include "../base/Mutex.h"
#include "../base/Timestamp.h"

#include "Callbacks.h"
#include "Channel.h"

namespace muduo {
    namespace net {

        //在另外一个类中使用该类的指针，可以通过前向声明来实现。
// 例如，可以在头文件中声明一个类的指针类型，而不必包含该类的定义。
// 然后，在另一个类中可以使用该指针类型，而无需包含声明该类的头文件。
// 这种写法可以减少头文件之间的依赖关系，从而提高编译效率，并避免循环包含的问题。

        // 这种写法通常被称为“前向声明”(forward declaration)。
        // 前向声明常用于解决头文件之间的依赖关系或避免循环包含。
        class EventLoop;
        class Timer;
        class TimerId;

        class TimerQueue : noncopyable{
        public:
            explicit TimerQueue(EventLoop* loop);
            ~TimerQueue();

            // EventLoop使用
            // addTimer()是供EventLoop使用并被封装成更好用的runAt()、runAfter()、runEvery()等。
            TimerId addTimer(TimerCallback cb,
                             Timestamp when,
                             double interval);

            void cancel(TimerId timerId);



        private:
            /*
             * TimerQueue虽然带了Queue，但是组织Timer的数据结构实际使用为std::set，
             * 其key为std::pair<TimeStamp, Timer*>，可以处理相同到期时间的Timer，保证相同到期时间Timer形成的key地址不同，不必使用std::mutiset。
             *
             * 另外，Timer在std::set<std::pair<TimeStamp, Timer*>>中是按照到期时间先后排好序的，操作的复杂度也是O(logN)。
             */

            // std::set 底层一般使用平衡二叉搜索树实现的 比如红黑树

            typedef std::pair<Timestamp, Timer*> Entry;
            typedef std::set<Entry> TimerList;

            typedef std::pair<Timer*, int64_t> ActiveTimer;
            typedef std::set<ActiveTimer> ActiveTimerSet;

            void addTimerInLoop(Timer* timer);
            void cancelInLoop(TimerId timerId);

            // timerfd 报警时调用
            void handleRead();

            // 移出所有过期的计时器
            std::vector<Entry> getExpired(Timestamp now);
            void reset(const std::vector<Entry>& expired, Timestamp now);

            bool insert(Timer* timer);


            EventLoop* loop_;            			// 所属的EventLoop
            const int timerfd_;				    // 当前定时器队列管理的timerfd
            Channel timerfdChannel_;              // 当前定时器队列管理的timerfd
            // Timer list sorted by expiration
            TimerList timers_; 					// 按到期时间先后排序好的定时器队列

            // for cancel()
            ActiveTimerSet activeTimers_;					// 还未到超时时间的定时器
            bool callingExpiredTimers_; /* atomic */        // 是否正在处理超时的定时器
            ActiveTimerSet cancelingTimers_;                // 保存的是被取消的定时器


        };




    } // net
}


#endif //MUDUO_NET_TIMERQUEUE_H
