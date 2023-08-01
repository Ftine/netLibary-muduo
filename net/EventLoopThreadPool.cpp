//
// Created by ftion on 2023/6/8.
//

#include "EventLoopThreadPool.h"
#include "EventLoopThread.h"
#include "EventLoop.h"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg)
        : baseLoop_(baseLoop),
          name_(nameArg),
          started_(false),
          numThreads_(0),
          next_(0)
{
}

EventLoopThreadPool::~EventLoopThreadPool()
{
    // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::start(const ThreadInitCallback &cb) {
    assert(!started_);
    baseLoop_->assertInLoopThread();

    started_ = true;

    for(int i = 0; i < numThreads_; i++){
        char buf[name_.size()+32];
        snprintf(buf, sizeof buf, "%s%d", name_.c_str(), i);

        // 调用回调函数创建一个EventLoopThread
        EventLoopThread* t = new EventLoopThread(cb,name_);
        // EventLoopThread进入队列
        threads_.push_back(std::unique_ptr<EventLoopThread>(t));
        // EventLoop进入队列
        loops_.push_back(t->startLoop());
    }

    /*
     * 如果 numThreads_ 为 0，说明只需要一个 EventLoop，
     * 此时会将 baseLoop_ 传递给回调函数 cb，
     * 并在 baseLoop_ 进入事件循环之前调用 cb。
     */
    if(numThreads_ == 0 && cb){
        cb(baseLoop_);
    }
}


EventLoop *EventLoopThreadPool::getNextLoop() {
    baseLoop_->assertInLoopThread();
    assert(started_);
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop =loops_[next_];
        ++next_;
        if(implicit_cast<size_t>(next_) >= loops_.size()){
            next_ = 0;
        }
    }

    return loop;
}


EventLoop *EventLoopThreadPool::getLoopForHash(size_t hashCode) {
    baseLoop_->assertInLoopThread();
    EventLoop* loop = baseLoop_;

    if(!loops_.empty()){
        loop = loops_[hashCode%loops_.size()];
    }
    return  loop;
}

std::vector<EventLoop *> EventLoopThreadPool::getAllLoops() {
    baseLoop_->assertInLoopThread();
    assert(started_);
    if(loops_.empty()){
        return std::vector<EventLoop*>(1, baseLoop_);
    }else{
        return loops_;
    }
}