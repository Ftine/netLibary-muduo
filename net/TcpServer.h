//
// Created by ftion on 2023/6/13.
//

#ifndef MUDUO_NET_TCPSERVER_H
#define MUDUO_NET_TCPSERVER_H

#include "../base/Atomic.h"
#include "../base/Types.h"
#include "TcpConnection.h"

#include <map>

namespace muduo{
    namespace net{
    class Acceptor;
    class EventLoop;
    class EventLoopThreadPool;

    class TcpServer: noncopyable{
    public:
        typedef std::function<void(EventLoop*)> ThreadInitCallback;

        enum Option
        {
            kNoReusePort,
            kReusePort,
        };

        //传入TcpServer所属的loop，本地ip，服务器名
        //TcpServer(EventLoop* loop, const InetAddress& listenAddr);
        TcpServer(EventLoop* loop,
                  const InetAddress& listenAddr,
                  const string& nameArg,
                  Option option = kNoReusePort);
        ~TcpServer();  // force out-line dtor, for std::unique_ptr members.

        const string& ipPort() const { return ipPort_; }
        const string& name() const { return name_; }
        EventLoop* getLoop() const { return loop_; }

        /// 设定线程池中线程的数量（sub reactor的个数），需在start()前调用
        // 0： 单线程(accept 和 IO 在一个线程)
        // 1： acceptor在一个线程，所有IO在另一个线程
        // N： acceptor在一个线程，所有IO通过round-robin分配到N个线程中
        void setThreadNum(int numThreads);
        // 线程池初始化后的回调函数
        void setThreadInitCallback(const ThreadInitCallback& cb)
        { threadInitCallback_ = cb; }
        /// 返回 -- EventLoopThreadPool
        std::shared_ptr<EventLoopThreadPool> threadPool()
        { return threadPool_; }

        //启动线程池-EventLoopThreadPool管理器，将Acceptor::listen()加入调度队列（只启动一次）
        void start();

        // 连接建立、断开，消息到达，消息完全写入tcp内核发送缓冲区 的回调函数
        // 非线程安全，但是都在IO线程中调用
        void setConnectionCallback(const ConnectionCallback& cb)
        { connectionCallback_ = cb; }
        void setMessageCallback(const MessageCallback& cb)
        { messageCallback_ = cb; }
        void setWriteCompleteCallback(const WriteCompleteCallback& cb)
        { writeCompleteCallback_ = cb; }


    private:

        /// Not thread safe, but in loop
        ///传给Acceptor，Acceptor会在有新的连接到来时调用->handleRead()
        void newConnection(int sockfd, const InetAddress& peerAddr);
        /// Thread safe.
        void removeConnection(const TcpConnectionPtr& conn);   //移除连接。
        /// Not thread safe, but in loop
        void removeConnectionInLoop(const TcpConnectionPtr& conn);  //将连接从loop中移除


        //string为TcpConnection名，用于找到某个连接。
        typedef std::map<string, TcpConnectionPtr> ConnectionMap;

        EventLoop* loop_; // Acceptor 所属的loop
        const string  ipPort_;
        const string name_;

        // 仅由TcpServer持有
        std::unique_ptr<Acceptor> acceptor_; // avoid revealing Acceptor
        std::shared_ptr<EventLoopThreadPool> threadPool_;
        // 用户的回调函数
        ConnectionCallback connectionCallback_;         // 连接状态（连接、断开）的回调
        MessageCallback messageCallback_;				  // 新消息到来的回调
        WriteCompleteCallback writeCompleteCallback_;   // 发送数据完毕(全部发给内核)的回调
        ThreadInitCallback threadInitCallback_;         // 线程池创建成功回调

        AtomicInt32 started_;
        // always in loop thread;
        int nextConnId_;  // 下一个连接ID， 用于生成TcpConnection的name;
        ConnectionMap  connections_;  // 当前连接的TCP的map<name,TcpConnectionPtr>

    };
    }
}

#endif //MUDUO_NET_TCPSERVER_H



