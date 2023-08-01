//
// Created by fight on 2023/5/30.
//

#include "TimerQueue.h"
#include "../base/Logging.h"

#include "Timer.h"
#include "TimerId.h"
#include "EventLoop.h"

#include <sys/timerfd.h>
#include <unistd.h>

namespace muduo{
    namespace net{
        namespace detail{

            // 目的 实现对 timerfd 的封装 不是使用原生接口


            int createTimerfd()
            {
                int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                               TFD_NONBLOCK | TFD_CLOEXEC);
                if (timerfd < 0)
                {
                    LOG_SYSFATAL << "Failed in timerfd_create";
                }
                return timerfd;
            }

            struct timespec howMuchTimeFromNow(Timestamp when)
            {
                int64_t microseconds = when.microSecondsSinceEpoch()
                                       - Timestamp::now().microSecondsSinceEpoch();
                if (microseconds < 100)
                {
                    microseconds = 100;
                }
                struct timespec ts;
                ts.tv_sec = static_cast<time_t>(
                        microseconds / Timestamp::kMicroSecondsPerSecond);
                ts.tv_nsec = static_cast<long>(
                        (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
                return ts;
            }

            void readTimerfd(int timerfd, Timestamp now)
            {
                uint64_t howmany;
                ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
                LOG_TRACE << "TimerQueue::handleRead() " << howmany << " at " << now.toString();
                if (n != sizeof howmany)
                {
                    LOG_ERROR << "TimerQueue::handleRead() reads " << n << " bytes instead of 8";
                }
            }

            void resetTimerfd(int timerfd, Timestamp expiration)
            {
                // wake up loop by timerfd_settime()
                struct itimerspec newValue;
                struct itimerspec oldValue;
                memZero(&newValue, sizeof newValue);
                memZero(&oldValue, sizeof oldValue);
                newValue.it_value = howMuchTimeFromNow(expiration);
                int ret = ::timerfd_settime(timerfd, 0, &newValue, &oldValue);
                if (ret)
                {
                    LOG_SYSERR << "timerfd_settime()";
                }
            }

        } // detail
    } // net
} // muduo


using namespace muduo;
using namespace muduo::net;
using namespace muduo::net::detail;

/*
 * 构造时传递其所属的EventLoop，创建一个timerfd并构造一个channel。在构造函数中，
 * 设置channel的可读事件的回调，设置关注channel的可读事件，
 * 并交由EventLoop管理，
 * 加入Poller监听队列。
 */

TimerQueue::TimerQueue(EventLoop* loop)
        : loop_(loop),
          timerfd_(createTimerfd()),
          timerfdChannel_(loop, timerfd_),    //timerfd相关的channel
          timers_(),
          callingExpiredTimers_(false)
{
    timerfdChannel_.setReadCallback(std::bind(&TimerQueue::handleRead, this));

    timerfdChannel_.enableReading();    //设置关注读事件，并且加入epoll队列
}

// 在析构函数中，必须注销 channel的所有事件，
// 并手动调用remove，从EventLoop、Poller移除监听。
TimerQueue::~TimerQueue()
{
    timerfdChannel_.disableAll();    // 设置不关注channel上的时间
    timerfdChannel_.remove();        // unregister channel
    ::close(timerfd_);               // 需要关闭fd
    // do not remove channel, since we're in EventLoop::dtor();
    for (const Entry& timer : timers_){
        delete timer.second;
    }
}

// 线程安全的，可以跨线程调用。创建一个Timer并添加到定时器队列中
TimerId TimerQueue::addTimer(TimerCallback cb, Timestamp when, double interval)
{
    Timer* timer = new Timer(std::move(cb), when, interval); // 创建一个Timer
    loop_->runInLoop(std::bind(&TimerQueue::addTimerInLoop, this, timer)); // 添加到定时器队列
    return TimerId(timer, timer->sequence()); // 返回一个TimerId对象
}

// 取消一个定时器
void TimerQueue::cancel(TimerId timerId)
{
    loop_->runInLoop(std::bind(&TimerQueue::cancelInLoop, this, timerId));
}

void TimerQueue::addTimerInLoop(Timer* timer)
{
    loop_->assertInLoopThread();
    // 插入一个新的定时器，判断是否需要更新timerfd的超时时间
    bool earliestChanged = insert(timer);

    if (earliestChanged){ // 需要更新超时事件
        resetTimerfd(timerfd_, timer->expiration()); // 重置，重新设定timerfd的超时时间
    }
}

void TimerQueue::cancelInLoop(TimerId timerId)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());

    // 通过TimerId从未超时的队列中activeTimers_找Timer
    ActiveTimer timer(timerId.timer_, timerId.sequence_);
    ActiveTimerSet::iterator it = activeTimers_.find(timer);

    if (it != activeTimers_.end()){
        // timers_队列中找到Timer则移除，并且释放Timer对象
        size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
        assert(n == 1); (void)n;
        delete it->first; // FIXME: no delete please
        activeTimers_.erase(it);
    }
    else if (callingExpiredTimers_){  // 没有找到定时器，并且当前处于处理超时的定时器状态中
        cancelingTimers_.insert(timer); // 放入到待取消的定时器队列
    }
    assert(timers_.size() == activeTimers_.size());
}

bool TimerQueue::insert(Timer* timer)
{
    loop_->assertInLoopThread();
    assert(timers_.size() == activeTimers_.size());
    bool earliestChanged = false;  // 最早到期的超时时间是否发生改变
    Timestamp when = timer->expiration();    // 新加入的定时器超时时间
    TimerList::iterator it = timers_.begin(); // 现有定时器中距离此刻最先发生超时的定时器

    // 新加入定时器的超时时间要小于现有最先发生超时的定时器的超时时间
    if (it == timers_.end() || when < it->first){
        earliestChanged = true;
    }
    { //添加到timers_中
        std::pair<TimerList::iterator, bool> result = timers_.insert(Entry(when, timer));
        assert(result.second); (void)result;
    }
    {  // 添加到activeTimers_中
        std::pair<ActiveTimerSet::iterator, bool> result
                = activeTimers_.insert(ActiveTimer(timer, timer->sequence()));
        assert(result.second); (void)result;
    }

    assert(timers_.size() == activeTimers_.size());
    return earliestChanged;
}

//TimerChannel的回调函数，是当timerfd定时器超时的时候，就会调用这个函数。
void TimerQueue::handleRead()
{
    loop_->assertInLoopThread();
    Timestamp now(Timestamp::now());
    readTimerfd(timerfd_, now);     // 从timerfd描述符进行读操作，避免重复触发超时事件

    // 获取当前时刻超时的定时器列表
    std::vector<Entry> expired = getExpired(now);

    callingExpiredTimers_ = true;   // 开始处理到期的定时器
    cancelingTimers_.clear();       // 清理要取消的的定时器（不再会执行的定时器）
    // safe to callback outside critical section
    for (const Entry& it : expired){
        it.second->run();    // 执行定时器超时的回调函数
    }
    callingExpiredTimers_ = false;

    reset(expired, now); // 处理超时的定时器，重新激活还是删除
}


//从timers_中移除已到期的Timer，并通过vector返回，编译器会进行RVO优化，不用考虑性能。
std::vector<TimerQueue::Entry> TimerQueue::getExpired(Timestamp now){
    assert(timers_.size()==activeTimers_.size());

    // 存储 已到期 的 Timer
    std::vector<Entry> expired;

    // 这段代码的作用是在timers_中查找第一个超时时间大于等于sentry的定时器对象的迭代器，
    // 时间是从小到大排序的有序的 如果比这个小 那么时间都用完了 已经超时了，大于这个时间就还有时间可以运行
    // 并将其赋值给end。如果没有找到这样的定时器对象，则将end赋值为timers_.end()。

    // 这段代码使用assert函数进行断言，确保end的值要么等于timers_.end()，
    // 要么当前时间now小于end指向的定时器的超时时间。这个assert函数的作用是确保查找操作的正确性，
    // 如果不满足条件，就会触发断言错误，从而中断程序的执行。
    Entry  sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
    TimerList::iterator end = timers_.lower_bound(sentry);
    assert(end == timers_.end() || now < end->first);

    // 复制 已到期 的Timer到expired 然后 删除timer中的
    std::copy(timers_.begin(), end, back_inserter(expired));
    timers_.erase(timers_.begin(), end);

    // 从activeTimers_中移除到期的定时器
    for (const Entry& it : expired)
    {
        ActiveTimer timer(it.second, it.second->sequence());
        size_t n = activeTimers_.erase(timer);
        assert(n == 1); (void)n;
    }

    assert(timers_.size() == activeTimers_.size());
    return expired;

}


//对于超时的定时器，进行重置操作，两种情况
//（1）需要重复执行的，重新设定超时事件，并作为新的定时器添加到cancelingTimers_中
//（2）不需要重复执行的，销毁Timer对象即可。

//处理完超时的定时器之后，重设 timerfd--Timer 的超时时间。
void TimerQueue::reset(const std::vector<Entry>& expired, Timestamp now)
{
    Timestamp nextExpire;
    for (const Entry& it : expired){
        ActiveTimer timer(it.second, it.second->sequence());
        if (it.second->repeat() && cancelingTimers_.find(timer) == cancelingTimers_.end()){
            it.second->restart(now);  // 重新启动定时器
            insert(it.second);
        } else {
            // FIXME move to a free list
            delete it.second; // FIXME: no delete please
        }
    }

    if (!timers_.empty())   { nextExpire = timers_.begin()->second->expiration(); }
    if (nextExpire.valid()) { resetTimerfd(timerfd_, nextExpire); }
}

