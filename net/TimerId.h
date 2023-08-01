//
// Created by fight on 2023/5/30.
//

#ifndef MUDUO_NET_TIMERID_H
#define MUDUO_NET_TIMERID_H

#include <cstdint>
#include "../base/copyable.h"

namespace muduo{
    namespace net{
        class Timer;

        // TimerId结构非常简单，封装一个Timer指针和其序列号，它被设计用来取消Timer的。
        //
        //使用 pair<Timer*, int64_t>来表示一个唯一的定时器，
        // 因为Timer*无法表示区分前后两次创建但可能地址相同的情况，增加一个id来确保唯一性。
        class TimerId : public muduo::copyable
        {
        public:
            TimerId()
                    : timer_(nullptr),
                      sequence_(0)
            {
            }

            TimerId(Timer* timer, int64_t seq)
                    : timer_(timer),
                      sequence_(seq)
            {
            }

            // default copy-ctor, dtor and assignment are okay

            friend class TimerQueue;

        private:
            Timer* timer_;
            int64_t sequence_;

        };
    } // net
}

#endif //MUDUO_NET_TIMERID_H
