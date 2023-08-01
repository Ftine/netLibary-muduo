//
// Created by fight on 2023/5/30.
//

#include "Timer.h"

using namespace muduo;
using namespace muduo::net;

// 声明
AtomicInt64 Timer::s_numCreated_;

void Timer::restart(Timestamp now)
{
    if (repeat_)
    {
        expiration_ = addTime(now, interval_);
    }
    else
    {
        expiration_ = Timestamp::invalid();
    }
}
