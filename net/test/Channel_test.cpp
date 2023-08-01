//
// Created by fight on 2023/6/2.
//

#include "../../base/Logging.h"
#include "../Channel.h"
#include "../EventLoop.h"

#include <functional>
#include <map>

#include <stdio.h>
#include <unistd.h>
#include <sys/timerfd.h>

using namespace muduo;
using namespace muduo::net;

void print(const char* msg){
    static std::map<const char*, Timestamp> lasts;
    Timestamp& last = lasts[msg];
    Timestamp now = Timestamp::now();
    printf("%s tid %d %s delay %f\n",
           now.toString().c_str(), CurrentThread::tid(),
           msg, timeDifference(now, last));
    last = now;
}

namespace muduo{
    namespace net{
        namespace detail{
            int createTimerfd();
            void readTimerfd(int timerfd, Timestamp now);
        }
    }
}

// 使用相对时间，不受挂钟变化的影响。
// 周期定时器

/*
 * 首先创建了一个PeriodicTimer类，该类封装了一个定时器，用于定期执行指定的回调函数。
 * 然后使用EventLoop类的runEvery方法向EventLoop中添加另一个定时任务，用于定期执行另一个回调函数。
 * 最后，调用EventLoop的loop方法，进入事件循环，等待定时任务的到来。
 */
class PeriodicTimer{
public:
    PeriodicTimer(EventLoop* loop, double interval, const TimerCallback& cb)
            : loop_(loop),
              timerfd_(muduo::net::detail::createTimerfd()),
              timerfdChannel_(loop, timerfd_),
              interval_(interval),
              cb_(cb)
    {
        timerfdChannel_.setReadCallback(std::bind(&PeriodicTimer::handleRead, this));

        timerfdChannel_.enableReading(); // 注意：这里会调用EvenetLoop::updateChannel
    }

    void start()
    {
        struct itimerspec spec;
        memZero(&spec, sizeof spec);
        spec.it_interval = toTimeSpec(interval_);
        spec.it_value = spec.it_interval;
        int ret = ::timerfd_settime(timerfd_, 0 /* relative timer */, &spec, NULL);
        if (ret)
        {
            LOG_SYSERR << "timerfd_settime()";
        }
    }

    ~PeriodicTimer()
    {
        timerfdChannel_.disableAll();
        timerfdChannel_.remove();
        ::close(timerfd_);
    }


private:
    void handleRead()
    {
        loop_->assertInLoopThread();
        muduo::net::detail::readTimerfd(timerfd_, Timestamp::now());
        if (cb_)
            cb_();
    }

    static struct timespec toTimeSpec(double seconds)
    {
        struct timespec ts;
        memZero(&ts, sizeof ts);
        const int64_t kNanoSecondsPerSecond = 1000000000;
        const int kMinInterval = 100000;
        int64_t nanoseconds = static_cast<int64_t>(seconds * kNanoSecondsPerSecond);
        if (nanoseconds < kMinInterval)
            nanoseconds = kMinInterval;
        ts.tv_sec = static_cast<time_t>(nanoseconds / kNanoSecondsPerSecond);
        ts.tv_nsec = static_cast<long>(nanoseconds % kNanoSecondsPerSecond);
        return ts;
    }

    EventLoop* loop_;
    const int timerfd_;
    Channel timerfdChannel_;
    const double interval_; // in seconds
    TimerCallback cb_;
};

int main()
{
    LOG_INFO << "pid = " << getpid() << ", tid = " << CurrentThread::tid()
             << " Try adjusting the wall clock, see what happens.";
    EventLoop loop;
    PeriodicTimer timer(&loop, 1, std::bind(print, "PeriodicTimer"));
    timer.start();
    loop.runEvery(1, std::bind(print, "EventLoop::runEvery"));
    loop.loop();
}

