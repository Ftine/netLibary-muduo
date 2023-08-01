//
// Created by fight on 2023/5/30.
//

#ifndef MUDUO_NET_TIMER_H
#define MUDUO_NET_TIMER_H

#include "../base/Atomic.h"
#include "../base/Timestamp.h"
#include "../base/noncopyable.h"
#include "Callbacks.h"

namespace muduo{
    namespace net{
        /*
         * Timer封装了定时器的一些参数，例如超时回调函数、超时时间、定时器是否重复、重复间隔时间、定时器的序列号。
         * 其函数大都是设置这些参数，run()用来调用回调函数，restart()用来重启定时器（如果设置为重复）
         */
        class Timer : noncopyable
        {
        public:
            Timer(TimerCallback cb, Timestamp when, double interval)
                    : callback_(std::move(cb)),    // 超时执行的回调函数
                      expiration_(when),           // 超时事件
                      interval_(interval),         // 超时重复执行的时间间隔
                      repeat_(interval > 0.0),     // 重复执行的标志位
                      sequence_(s_numCreated_.incrementAndGet()) // 当前定时器的的全局唯一序号
            { }

            void run() const { callback_(); } // 执行超时回调函数

            Timestamp expiration() const  { return expiration_; }  // 返回超时的时刻
            bool repeat() const { return repeat_; }                // 返回是否重复
            int64_t sequence() const { return sequence_; }         // 返回定时器的序号

            void restart(Timestamp now);  // 重新设定定时器

            static int64_t numCreated() { return s_numCreated_.get(); } // 返回最新的定时器序号

        private:
            const TimerCallback callback_;    // 定时器回调函数
            Timestamp expiration_;            // 定时器超时的时间
            const double interval_;           // 定时器重复执行时间间隔，如不重复，设为非正值
            const bool repeat_;               // 定时器是都重复执行
            const int64_t sequence_;          // 定时器的全局序号

            static AtomicInt64 s_numCreated_; // 定时器计数值
        };

    }
}

#endif //MUDUO_NET_TIMER_H
