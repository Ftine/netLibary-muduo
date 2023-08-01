//
// Created by ftion on 2023/6/13.
//

#include "TcpServer.h"
#include "../base/Logging.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "SocketsOpts.h"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const string& nameArg,
                     Option option)
        : loop_(CHECK_NOTNULL(loop)),
          ipPort_(listenAddr.toIpPort()),		// 服务端监听的地址
          name_(nameArg),
          acceptor_(new Acceptor(loop, listenAddr, option == kReusePort)),  // 处理新连接的Acceptor
          threadPool_(new EventLoopThreadPool(loop, name_)), // 线程池
          connectionCallback_(defaultConnectionCallback),    // 提供给用户的 连接、断开 的回调
          messageCallback_(defaultMessageCallback),          // 提供给用户的 新消息到来 的回调
          nextConnId_(1)									   // 下一个连接到来的序号
{
    // 设置Acceptor处理新连接的回调函数
    acceptor_->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, _1, _2));
}

TcpServer::~TcpServer()
{
    loop_->assertInLoopThread();
    LOG_TRACE << "TcpServer::~TcpServer [" << name_ << "] destructing";

    for (auto& item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();   // 引用计数减1，不代表已经释放
        // 调用TcpConnection::connectDestroyed
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    }
}

/*
 * 函数setThreadNum()必须在调用start()前使用，
 * 若不设置线程数量，则默认单线程模式处理。
 * start()函数启动Acceptor::listen()开始监听新连接到来的事件，
 * 一旦有新连接的事件就触发回调TcpServer::newConnection。
 */

void TcpServer::setThreadNum(int numThreads)
{
    assert(0 <= numThreads);
    threadPool_->setThreadNum(numThreads);
}

void TcpServer::start()
{
    if (started_.getAndSet(1) == 0)
    {
        threadPool_->start(threadInitCallback_);

        assert(!acceptor_->listenning());
        loop_->runInLoop(std::bind(&Acceptor::listen, get_pointer(acceptor_)));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress &peerAddr) {
    loop_->assertInLoopThread();
    // 使用round-robin策略从线程池中获取一个loop
    EventLoop* ioLoop = threadPool_->getNextLoop();

    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;

    LOG_INFO << "TcpServer::newConnection [" << name_
             << "] - new connection [" << connName
             << "] from " << peerAddr.toIpPort();
    InetAddress localAddr(sockets::getLocalAddr(sockfd));

    // FIXME poll with zero timeout to double confirm the new connection
    // FIXME use make_shared if necessary
    // 创建一个TcpConnection对象，表示每一个已连接
    TcpConnectionPtr conn(new TcpConnection(ioLoop,    // 当前连接所属的loop
                                            connName,  // 连接名称
                                            sockfd,    // 已连接的socket
                                            localAddr, // 本端地址
                                            peerAddr));// 对端地址
    // 当前连接加入到connection map中
    connections_[connName] = conn;
    // 设置TcpConnection上的事件回调
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, _1)); // FIXME: unsafe
    // 回调执行TcpConnection::connectEstablished()，确认当前已连接状态，
    // 在Poller中注册当前已连接socket上的IO事件
    ioLoop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));

}


void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    // FIXME: unsafe
    loop_->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    LOG_INFO << "TcpServer::removeConnectionInLoop [" << name_
             << "] - connection " << conn->name();
    size_t n = connections_.erase(conn->name());  // 通过name移除coon
    (void)n;
    assert(n == 1);
    // 在conn所在的loop中执行TcpConnection::connectDestroyed
    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
}


