//
// Created by ftion on 2023/6/8.
//

#ifndef MUDUO_NET_EVENTLOOPTHREADPOOL_H
#define MUDUO_NET_EVENTLOOPTHREADPOOL_H

#include "../base/noncopyable.h"
#include "../base/Types.h"

#include <functional>
#include <memory>
#include <vector>

namespace muduo{
    namespace net{
        class EventLoop;
        class EventLoopThread;

        /*
         * EventLoopThreadPool是封装了多个EventLoopThread的线程池，管理所有客户端上的IO事件，每个线程都有唯一一个事件循环。
         * 主线程的EventLoopThread负责新的客户端连接，线程池中的EventLoopThread负责客户端的IO事件，客户端IO事件的分配按照一定规则执行。
         */
        class EventLoopThreadPool:noncopyable{
        public:
            typedef std::function<void(EventLoop*)> ThreadInitCallback;
            EventLoopThreadPool(EventLoop* baseLoop, const string& nameArg);
            ~EventLoopThreadPool();

            void setThreadNum(int numThreads){ numThreads_ = numThreads;};

            // 启动EventLoopThreadPool
            void start(const ThreadInitCallback& cb = ThreadInitCallback());

            // 启动后有效，使用round-robin策略获取线程池中的EventLoop对象
            EventLoop* getNextLoop();

            // 使用固定的hash方法，获取相同的线程对象
            EventLoop* getLoopForHash(size_t hashCode);

            // 返回所有EvenetLoop，含baseloop
            std::vector<EventLoop*> getAllLoops();

            bool started() const { return started_; }

            const string& name() const { return name_; }

        private:
            EventLoop* baseLoop_;		// Acceptor所属EventLoop   就是主循环
            string name_;				// 线程池名称
            bool started_;			    // 是否已经启动
            int numThreads_;			// 线程数
            int next_;				    // 新连接到来，所选择的EventLoop对象下标
            std::vector<std::unique_ptr<EventLoopThread>> threads_; // EventLoopThread--I/O线程列表
            std::vector<EventLoop*> loops_;	// EventLoop列表

        };
    }
}

#endif //MUDUO_NET_EVENTLOOPTHREADPOOL_H
