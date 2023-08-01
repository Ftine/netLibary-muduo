//
// Created by ftion on 2023/6/8.
//

#include "EventLoopThread.h"
#include "EventLoop.h"

using namespace muduo;
using namespace muduo::net;

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb, const string &name)
:loop_(NULL),exiting_(false), thread_(std::bind(&EventLoopThread::threadFunc,this),name),
    mutex_(), cond_(mutex_),callback_(cb){}

// 析构，退出事件循环，关闭线程
EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != NULL) {
        loop_->quit();
        thread_.join();
    }
}

EventLoop* EventLoopThread::startLoop()
{
    assert(!thread_.started());
    thread_.start();			// 启动线程，执行线程函数

    EventLoop* loop = NULL;
    {
        MutexLockGuard lock(mutex_);
        while (loop_ == NULL){
            cond_.wait();   // 保证线程函数准备工作完成，EventLoop指针对象不为空
        }
        loop = loop_;
    }

    return loop;
}

void EventLoopThread::threadFunc()
{
    EventLoop loop;  // 线程中创建一个EventLoop对象

    if (callback_){
        callback_(&loop);  // 执行初始化回调函数
    }

    {
        MutexLockGuard lock(mutex_);
        loop_ = &loop;
        cond_.notify();  // 初始化成功，通知用户启动成功
    }

    loop.loop();		// 线程中执行事件循环
    //assert(exiting_);
    MutexLockGuard lock(mutex_);
    loop_ = NULL;
}


