//
// Created by fight on 2023/5/22.
//

#include "ThreadPool.h"
#include "Exception.h"

#include <sys/time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

using namespace muduo;

ThreadPool::ThreadPool(const string& nameArg)
        : mutex_(),
          notEmpty_(mutex_),
          notFull_(mutex_),
          name_(nameArg),
          maxQueueSize_(0),
          running_(false){

}

ThreadPool::~ThreadPool(){
    if (running_)
    {
        stop();
    }
}

void ThreadPool::start(int numThreads){
    assert(threads_.empty());
    running_ = true;
    threads_.reserve(numThreads);
    for (int i = 0; i < numThreads; ++i)
    {
        char id[32];
        snprintf(id, sizeof id, "%d", i+1);
        threads_.emplace_back(new muduo::Thread(
                std::bind(&ThreadPool::runInThread, this), name_+id)); // 穿件封装的线程对象
        threads_[i]->start(); // 调用 pthread_create
        /*
         * 使用new muduo::Thread(ThreadFunc, const string& )创建线程，
         * 第一个参数是利用std::bind()绑定得到的std::function<void()>的线程执行函数，第二个参数是 name_+id 。
         * 线程实际执行函数是当前类的成员函数 void ThreadPool::runInThread() ，
         * 当线程池中的每个线程启动时，都会执行该函数。
         */
    }
    if (numThreads == 0 && threadInitCallback_)
    {
        threadInitCallback_();
    }
}

void ThreadPool::stop(){
    {
        MutexLockGuard lock(mutex_);
        running_ = false;
        notEmpty_.notifyAll();
    }
    for (auto& thr : threads_)
    {
        thr->join();
    }
}

size_t ThreadPool::queueSize() const{
    MutexLockGuard lock(mutex_);
    return queue_.size();
}

void ThreadPool::run(Task task)
{
    if (threads_.empty()){
        task();   // 若线程池为空，则调用者将以阻塞的形式执行当前任务。
    }
    else{
        MutexLockGuard lock(mutex_); // 加锁
        while (isFull()){   // 对队列未满，直接入队
            notFull_.wait(); // 若任务队列满，解锁，阻塞等待任务队列有空位可入队
        } // 被唤醒
        assert(!isFull());

        queue_.push_back(std::move(task));  // 加锁，任务入队
        notEmpty_.notify();					// 通知队列不为空，唤醒取任务的线程（实际的消费者函数）
    }
}

bool ThreadPool::isFull() const{
    mutex_.assertLocked();
    return maxQueueSize_ > 0 && queue_.size() >= maxQueueSize_;
}

void ThreadPool::runInThread()
{
    try
    {
        if (threadInitCallback_){
            threadInitCallback_();
        }
        while (running_) {
            Task task(take());   // 获取任务
            if (task){	         // 可能为空 （线程池退出时唤醒，这里可能是一个空的任务）
                task();	         // 执行任务
            }
        }
    }
    catch (const Exception& ex){
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        fprintf(stderr, "stack trace: %s\n", ex.stackTrace());
        abort();
    }
    catch (const std::exception& ex){
        fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
        fprintf(stderr, "reason: %s\n", ex.what());
        abort();
    }
    catch (...){
        fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
        throw; // rethrow
    }
}


ThreadPool::Task ThreadPool::take(){
    MutexLockGuard lock(mutex_);   // 加锁
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty() && running_){
        notEmpty_.wait();  // 程序运行且队列为空时，解锁，阻塞等待有任务时唤醒
    } // 被唤醒
    Task task;			    // 加锁
    if (!queue_.empty()){     // 若此时队列不为空
        task = queue_.front();  // 从队列中取出一个任务
        queue_.pop_front();
        if (maxQueueSize_ > 0){
            notFull_.notify(); // 若队列设置最大允许任务数量大于0，发送队列不满的通知，唤醒生产者入队任务。
        }
    }
    return task;  // 可能为空（出现在线程池退出时）
}




