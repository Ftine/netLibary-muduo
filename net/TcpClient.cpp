//
// Created by ftion on 2023/6/15.
//

#include "TcpClient.h"

#include "../base/Logging.h"
#include "Connector.h"
#include "EventLoop.h"
#include "SocketsOpts.h"

#include <cstdio>

using namespace muduo;
using namespace muduo::net;

namespace muduo{
    namespace net{
        namespace detail{
            void removeConnection(EventLoop* loop, const TcpConnectionPtr& conn)
            {
                loop->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
            }

            void removeConnector(const ConnectorPtr& connector)
            {
                //connector->
            }
        }
    }
}

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& serverAddr,
                     const string& nameArg)
        : loop_(CHECK_NOTNULL(loop)),
          connector_(new Connector(loop, serverAddr)),	// 创建一个Connector
          name_(nameArg),
          connectionCallback_(defaultConnectionCallback),
          messageCallback_(defaultMessageCallback),
          retry_(false), 		// 默认不重连
          connect_(true), 	// 开始连接
          nextConnId_(1) 		// 当前连接的序号
{
    // 设置建立连接的回调函数
    connector_->setNewConnectionCallback(std::bind(&TcpClient::newConnection, this, _1));
    // FIXME setConnectFailedCallback
    LOG_INFO << "TcpClient::TcpClient[" << name_ << "] - connector " << get_pointer(connector_);
}

TcpClient::~TcpClient()
{
    LOG_INFO << "TcpClient::~TcpClient[" << name_ << "] - connector " << get_pointer(connector_);
    TcpConnectionPtr conn;
    bool unique = false;
    {
        MutexLockGuard lock(mutex_);
        unique = connection_.unique();  // 是否只有一个持有者
        conn = connection_;
    }
    if (conn)  // 连接已经建立成功，TcpConnectionPtr  不为空
    {
        assert(loop_ == conn->getLoop());
        // FIXME: not 100% safe, if we are in different thread
        CloseCallback cb = std::bind(&detail::removeConnection, loop_, _1);
        loop_->runInLoop(std::bind(&TcpConnection::setCloseCallback, conn, cb));
        if (unique){
            conn->forceClose();
        }
    }
    else{  // TcpConnectionPtr为空
        connector_->stop();   // 关闭Connector连接
        // FIXME: HACK
        loop_->runAfter(1, std::bind(&detail::removeConnector, connector_));
    }
}


void TcpClient::connect()
{
    // FIXME: check state
    LOG_INFO << "TcpClient::connect[" << name_ << "] - connecting to " << connector_->serverAddress().toIpPort();
    connect_ = true;
    connector_->start();  // 调用Connector::start，发起连接
}

// 若连接成功，则回调TcpClient::newConnection()函数。
// 参数是本地已建立连接的sockfd，通过它创建一个TcpConnection，用于后续消息的发送。
void TcpClient::newConnection(int sockfd) {
    loop_->assertInLoopThread();
    InetAddress peerAddr(sockets::getPeerAddr(sockfd));    // 获取对端的地址
    char buf[32];
    snprintf(buf, sizeof buf, ":%s#%d", peerAddr.toIpPort().c_str(), nextConnId_);
    ++nextConnId_;
    string connName = name_ + buf;  // 连接的名字

    InetAddress localAddr(sockets::getLocalAddr(sockfd));  // 获取本端的地址

    // 构造一个TcpConnection对象，并设置相应的回调函数
    TcpConnectionPtr conn(new TcpConnection(loop_,
                                            connName,
                                            sockfd,
                                            localAddr,
                                            peerAddr));
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(std::bind(&TcpClient::removeConnection, this, _1)); // FIXME: unsafe
    {
        MutexLockGuard lock(mutex_);
        connection_ = conn;
    }
    conn->connectEstablished();
}

// 断开连接，仅关闭写功能，仍然能接收对端消息。
void TcpClient::disconnect() {
    connect_ = false;
    {
        MutexLockGuard lock(mutex_);
        if(connection_){
            connection_->shutdown(); // 半关闭， 能继续完整接收对端的消息
        }
    }
}

// 关闭连接，则将完全关闭客户端，不能再进行收、发数据。
void TcpClient::stop()
{
    connect_ = false;
    connector_->stop();
}

// 移除连接
void TcpClient::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->assertInLoopThread();
    assert(loop_ == conn->getLoop());

    {
        MutexLockGuard lock(mutex_);
        assert(connection_ == conn);
        connection_.reset();
    }

    loop_->queueInLoop(std::bind(&TcpConnection::connectDestroyed, conn));
    if (retry_ && connect_)
    {
        LOG_INFO << "TcpClient::connect[" << name_ << "] - Reconnecting to "
                 << connector_->serverAddress().toIpPort();
        connector_->restart();
    }
}





