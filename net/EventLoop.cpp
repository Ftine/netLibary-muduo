//
// Created by ftion on 2023/5/31.
//
#include "../base/Logging.h"
#include "../base/Mutex.h"

#include "EventLoop.h"
#include "Channel.h"
#include "Poller.h"
#include "TimerQueue.h"
#include "SocketsOpts.h"


#include <algorithm>
#include <signal.h>
#include <sys/eventfd.h>
#include <unistd.h>


using namespace muduo;
using namespace muduo::net;

namespace {

    __thread EventLoop *t_loopInThisThread = 0;
    const int kPollTimeMs = 10000;

    int createEventfd(){
        int event_fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
        if(event_fd < 0){
            LOG_SYSERR << "Failed in createEventfd";
            abort();
        }
        return event_fd;
    }

#pragma GCC diagnostic ignored "-Wold-style-cast"
    class IgnoreSigPipe
    {
    public:
        IgnoreSigPipe()
        {
            ::signal(SIGPIPE, SIG_IGN);
            // LOG_TRACE << "Ignore SIGPIPE";
        }
    };
#pragma GCC diagnostic error "-Wold-style-cast"

    IgnoreSigPipe initObj;

}

EventLoop::EventLoop():
    looping_(false),
    threadId_(CurrentThread::tid()),
    eventHandling_(false),
    callingPendingFunctors_(false),
    iteration_(0),
    poller_(Poller::newDefaultPoller(this)),
    timerQueue_(new TimerQueue(this)),
    wakeupFd_(createEventfd()),
    wakeupChannel_(new Channel(this, wakeupFd_)),
    currentActiveChannel_(nullptr){

    LOG_TRACE << "EventLoop create" << this << " in thread" << threadId_;

    if(t_loopInThisThread){
        LOG_FATAL << "Another EventLoop " << t_loopInThisThread
                  << " exists in this thread " << threadId_;
    }else{
        t_loopInThisThread = this;
    }

    // 设置唤醒channel的可读事件回调，并注册到Poller中
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    // we are always reading the wakeupfd
    wakeupChannel_->enableReading();
}

/*
 * 析构函数中，需要从显示地调用注销在Poller的eventfd事件，且关闭文件描述符。
 * 由于成员poller_和timerQueue_使用std::unique_ptr管理，
 * 不用手动处理资源释放的问题，在各自的析构函数中进行了资源释放处理。
 */
EventLoop::~EventLoop(){
    LOG_DEBUG << "EventLoop " << this << " of thread " << threadId_
              << " destructs in thread " << CurrentThread::tid();

    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = NULL;
}


EventLoop* EventLoop::getEventLoopOfCurrentThread(){
    return t_loopInThisThread;
}

void EventLoop::abortNotInLoopThread()
{
    LOG_FATAL << "EventLoop::abortNotInLoopThread - EventLoop " << this
              << " was created in threadId_ = " << threadId_
              << ", current thread id = " <<  CurrentThread::tid();
}


void EventLoop::loop() {
    assert(!looping_);
    assertInLoopThread();
    looping_ = true;
    quit_ = false;
    LOG_TRACE << "EventLoop " << this << " start looping";

    while(!quit_){

        // 定期检查是否有就绪的IO事件（超时事件为kPollTimeMs = 10s）
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        ++iteration_;

        if (Logger::logLevel() <= Logger::TRACE)
        {
            printActiveChannels();
        }

        // 依次处理有就绪的IO事件的回调
        eventHandling_ = true;    // IO事件处理标志
        for (Channel* channel : activeChannels_)
        {
            currentActiveChannel_ = channel;
            currentActiveChannel_->handleEvent(pollReturnTime_);
        }
        currentActiveChannel_ = NULL ;
        eventHandling_ = false;
        doPendingFunctors();

    }
    LOG_TRACE << "EventLoop " << this << " stop looping";
    looping_ = false;
}

void EventLoop::quit()
{
    quit_ = true;
    // loop() 有可能只是执行 while(!quit_) --组赛了 并没有退出，
    // 然后 EventLoop 析构，然后我们正在访问一个无效的对象。
    // 可以在两个地方使用 mutex_ 来修复。
    if (!isInLoopThread())  // 处于阻塞阻塞状态 唤醒在结束loop
    {
        wakeup();
    }
}

void EventLoop::runInLoop(Functor cb)
{
    // 是当前线程的任务，就直接执行
    if(isInLoopThread()){

        cb();
    }else{
        // 不是当前线程的任务，加入任务队列等待执行
        queueInLoop(std::move(cb));
    }
}

/*
 * 由于IO线程平时阻塞在时间循环EventLoop::loop()中的poll函数上，
 * 为能让IO线程立刻执行用户回调，需要执行唤醒操作，有两种情况。
 * 第一种，是在其他线程调用，必须唤醒，否则阻塞直到超时才执行。
 * 第二种，在当前IO线程，但Loop正处理任务队列的任务，
 * Functor可能再调用queueInLoop，
 * 这种情况就必须执行wakeup，否则新加入到pendingFunctors_的cb就不能及时执行。
 */
void EventLoop::queueInLoop(Functor cb)
{
    {
        MutexLockGuard lock(mutex_);
        pendingFunctors_.push_back(std::move(cb));
    }

    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}


TimerId EventLoop::runAt(Timestamp time, TimerCallback cb)
{
    return timerQueue_->addTimer(std::move(cb), time, 0.0);
}

TimerId EventLoop::runAfter(double delay, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), delay));
    return runAt(time, std::move(cb));
}

TimerId EventLoop::runEvery(double interval, TimerCallback cb)
{
    Timestamp time(addTime(Timestamp::now(), interval));
    return timerQueue_->addTimer(std::move(cb), time, interval);
}

void EventLoop::cancel(TimerId timerId)
{
    return timerQueue_->cancel(timerId);
}


/*
 * Channel对象构造后，需要至少设置一个关注事件及其回调函数，设置关注事件内部会调用EventLoop::updateChannel(this)，将其注册到Poller上；
 * 更新关注事件也是。当关闭不再使用Channel，需要显示地关闭注册事件，并从Poller中注销，注销内部实现实际调用EventLoop::removeChannel(this)。
 * 因此，这三个操作实际都是在IO线程中执行的，不需要加锁。 三个函数的使用可以参考TimerQueue的构造函数和析构函数。
 */
void EventLoop::updateChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    poller_->updateChannel(channel);

}

void EventLoop::removeChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    if (eventHandling_)
    {
        assert(currentActiveChannel_ == channel ||
               std::find(activeChannels_.begin(), activeChannels_.end(), channel) == activeChannels_.end());
    }
    poller_->removeChannel(channel);   // 将channel的事件和回调从Poller中移除
}

bool EventLoop::hasChannel(Channel* channel)
{
    assert(channel->ownerLoop() == this);
    assertInLoopThread();
    return poller_->hasChannel(channel);
}


void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;

    {
        MutexLockGuard lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for (const Functor& functor : functors)
    {
        functor();
    }
    callingPendingFunctors_ = false;
}

void EventLoop::wakeup() const
{
    uint64_t one = 1;
    ssize_t n = sockets::write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::wakeup() writes " << n << " bytes instead of 8";
    }
}

void EventLoop::handleRead() const
{
    uint64_t one = 1;
    ssize_t n = sockets::read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR << "EventLoop::handleRead() reads " << n << " bytes instead of 8";
    }
}

void EventLoop::printActiveChannels() const
{
    for (const Channel* channel : activeChannels_)
    {
        LOG_TRACE << "{" << channel->reventsToString() << "} ";
    }
}