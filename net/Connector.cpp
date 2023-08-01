//
// Created by ftion on 2023/6/15.
//

#include "Connector.h"


#include "../base/Logging.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOpts.h"

#include <errno.h>

using namespace muduo;
using namespace muduo::net;

const int Connector::kMaxRetryDelayMs;

Connector::Connector(muduo::net::EventLoop *loop, const muduo::net::InetAddress &serverAddr)
        : loop_(loop),
          serverAddr_(serverAddr),
          connect_(false),
          state_(kDisconnected),
          retryDelayMs_(kMaxRetryDelayMs) {
    LOG_DEBUG << "ctor[" << this << "]";
}

Connector::~Connector()
{
    LOG_DEBUG << "dtor[" << this << "]";
    assert(!channel_);
}

// 准备连接
void Connector::start()
{
    connect_ = true;
    loop_->runInLoop(std::bind(&Connector::startInLoop, this)); // FIXME: unsafe
}

// Loop 执行
void Connector::startInLoop()
{
    loop_->assertInLoopThread();
    assert(state_ == kDisconnected);
    if (connect_){    // 开始连接标志位true的情况
        connect();    // 处理链接
    }
    else{
        LOG_DEBUG << "do not connect";
    }
}

void Connector::connect()
{
    int sockfd = sockets::createNonblockingOrDie(serverAddr_.family());
    int ret = sockets::connect(sockfd, serverAddr_.getSockAddr());
    int savedErrno = (ret == 0) ? 0 : errno;

    switch (savedErrno)
    {
        case 0:
        case EINPROGRESS:      	// 非阻塞不能立即返回，正在连接中，需后续socketfd可读检查SO_ERROR
        case EINTR:				// 连接被SIGNAL中断
        case EISCONN:			// 已连接状态
            connecting(sockfd);
            break;

        case EAGAIN:			// 本地地址处于使用状态
        case EADDRINUSE:		// 地址已被使用
        case EADDRNOTAVAIL:		// 无可用的本地端口用于连接
        case ECONNREFUSED:	    // 服务端地址上没有socket监听
        case ENETUNREACH:		// 网络不可达（防火墙？）
            retry(sockfd);
            break;

        case EACCES:
        case EPERM:				// 连接到广地地址但未设置该标志位；本地防火墙规则不允许连接
        case EAFNOSUPPORT:		// 不支持当前sa_family
        case EALREADY:			// socket是非阻塞的，但前一个连接请求尚未处理结束
        case EBADF:  			// sockfd参数在描述符表中无效
        case EFAULT:			// socket数据结构不在用户地址空间
        case ENOTSOCK:			// fd不是sokcet类型
            LOG_SYSERR << "connect error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            break;

        default:
            LOG_SYSERR << "Unexpected error in Connector::startInLoop " << savedErrno;
            sockets::close(sockfd);
            // connectErrorCallback_();
            break;
    }

}

/* 连接中的处理
 * 开始连接中的处理过程，设置当前连接状态kConnecting，将当前socketfd构造成Channel，
 * 并注册可写、错误事件到Poller。通过Poller监听的可读事件进一步判断连接状态。
 */
void Connector::connecting(int sockfd)
{
    setState(kConnecting);  // 设置状态
    assert(!channel_);
    // 重设channel_，设置可写事件回调、错误事件回调
    channel_.reset(new Channel(loop_, sockfd));
    channel_->setWriteCallback(std::bind(&Connector::handleWrite, this)); // FIXME: unsafe
    channel_->setErrorCallback(std::bind(&Connector::handleError, this)); // FIXME: unsafe

    // channel_->tie(shared_from_this()); is not working,
    // as channel_ is not managed by shared_ptr
    channel_->enableWriting();   // 注册到Poller
}

/*
 * 监听到socketfd的可读事件，确认当前连接是否成功建立，如果当前状态是kDisconnected，则不进行处理。
 * 如果当前状态是kConnecting连接中的状态，重置channel_，并从socketfd获取SO_ERROR。
 * 如果SO_ERROR值不为零有错误，或者是自连接，则尝试retry(sockfd)重连；
 * SO_ERROR值为0表示连接成功，正常进行回调传递已连接socketfd给TcpClient。
 * 在处理错误事件中处理状态是kConnecting的情况，重置channel_并尝试重连。
 */

void Connector::handleWrite() {
    LOG_TRACE << "Connector::handleWrite" << state_;

    if(state_ == kConnecting){ // 当前是kConnecting连接中的状态
        int sockfd = removeAndResetChannel(); // 当前是kConnecting连接中的状态
        int err = sockets::getSocketError(sockfd); // 获取上一次错误

        if(err){
            // 连接失败
            LOG_WARN << "Connector::handleWrite - SO_ERROR = " << err << " " << strerror_tl(err);
            retry(sockfd); //重新连接
        }else if(sockets::isSelfConnect(sockfd)){ //自连接
            LOG_WARN << "Connector::handleWrite - Self connect";
            retry(sockfd);
        } else{ //  错误值0， 连接成功
            setState(kConnected);    // 设置为kConnected已连接状态
            if (connect_){
                newConnectionCallback_(sockfd); // 回调TcpClient的连接成功回调，返回已连接socketfd
            }
            else{
                sockets::close(sockfd); // 若不进行连接，则关闭当前已连接socketfd
            }
        }
    }else {
        // what happened?
        assert(state_ == kDisconnected);
    }
}


void Connector::handleError()
{
    LOG_ERROR << "Connector::handleError state=" << state_;
    if (state_ == kConnecting){
        int sockfd = removeAndResetChannel();  // 重置
        int err = sockets::getSocketError(sockfd); // 尝试重连
        LOG_TRACE << "SO_ERROR = " << err << " " << strerror_tl(err);
        retry(sockfd);
    }
}

// 移除并重置Channel
// 当出错、尝试重连，都需要重置channel，
// 因为当前socketfd不能再重复使用，
// 但是这里不进行close关闭socketfd，在retry()中关闭。
int Connector::removeAndResetChannel()
{
    channel_->disableAll();
    channel_->remove();
    int sockfd = channel_->fd();
    // Can't reset channel_ here, because we are inside Channel::handleEvent
    loop_->queueInLoop(std::bind(&Connector::resetChannel, this)); // FIXME: unsafe
    return sockfd;
}

void Connector::resetChannel()
{
    channel_.reset();
}

/* 尝试重连
 * 尝试重连，关闭上一次使用的socketfd，设置连接状态为kDisconnected。
 * 重连使用定时器处理，每次尝试重连的时间都是上一次重连时间的两倍。
 *
 * 关闭socket，是为了处理断开连接调用stop()的情况，统一接口。
 */

void Connector::retry(int sockfd) {
    sockets::close(sockfd);
    setState(kDisconnected);
    if(connect_){
        LOG_INFO << "Connector::retry - Retry connecting to " << serverAddr_.toIpPort()
                 << " in " << retryDelayMs_ << " milliseconds. ";
        loop_->runAfter(retryDelayMs_/1000.0, std::bind(&Connector::startInLoop,shared_from_this()));
        retryDelayMs_ = std::min(retryDelayMs_ * 2, kMaxRetryDelayMs);
    } else{
        LOG_DEBUG << "do not connect";
    }
}

void Connector::stop()
{
    connect_ = false;   // 设置不连接
    loop_->queueInLoop(std::bind(&Connector::stopInLoop, this)); // FIXME: unsafe
    // FIXME: cancel timer
}

void Connector::stopInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnecting)  // 如果处于连接中
    {
        setState(kDisconnected); // 设置状态
        int sockfd = removeAndResetChannel();  // 重置
        retry(sockfd);  // 由于设置connect_为faslse， 这里重连操作实际不处理
    }
}

void Connector::restart()
{
    loop_->assertInLoopThread();
    setState(kDisconnected);
    retryDelayMs_ = kInitRetryDelayMs;  // 重置初始的重连超时时间
    connect_ = true;
    startInLoop();	// 准备开始连接
}



