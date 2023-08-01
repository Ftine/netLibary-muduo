//
// Created by fight on 2023/6/11.
//

#include "TcpConnection.h"

#include "../base/Logging.h"
#include "../base/WeakCallback.h"
#include "Channel.h"
#include "EventLoop.h"
#include "SocketsOpts.h"
#include "Socket.h"


#include <errno.h>

using namespace muduo;
using namespace muduo::net;

void muduo::net::defaultConnectionCallback(const TcpConnectionPtr& conn)
{
    LOG_TRACE << conn->localAddress().toIpPort() << " -> "
              << conn->peerAddress().toIpPort() << " is "
              << (conn->connected() ? "UP" : "DOWN");
    // do not call conn->forceClose(), because some users want to register message callback only.
}

void muduo::net::defaultMessageCallback(const TcpConnectionPtr&,
                                        Buffer* buf,
                                        Timestamp)
{
    buf->retrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& nameArg,
                             int sockfd,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
        : loop_(CHECK_NOTNULL(loop)),
          name_(nameArg),
          state_(kConnecting),
          reading_(true),
          socket_(new Socket(sockfd)), 			// 已连接socketfd封装的Socket对象
          channel_(new Channel(loop, sockfd)),    // 构造的channel
          localAddr_(localAddr),
          peerAddr_(peerAddr),
          highWaterMark_(64*1024*1024)   // 缓冲区数据最大64M
{
    // 向Channel对象注册可读事件
    channel_->setReadCallback(std::bind(&TcpConnection::handleRead, this, _1));
    channel_->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    channel_->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    channel_->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    LOG_DEBUG << "TcpConnection::ctor[" <<  name_ << "] at " << this << " fd=" << sockfd;
    // 设置保活机制
    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_DEBUG << "TcpConnection::dtor[" <<  name_ << "] at " << this
              << " fd=" << channel_->fd()
              << " state=" << stateToString();
    assert(state_ == kDisconnected);
}

bool TcpConnection::getTcpInfo(struct tcp_info* tcpi) const
{
    return socket_->getTcpInfo(tcpi);
}

string TcpConnection::getTcpInfoString() const
{
    char buf[1024];
    buf[0] = '\0';
    socket_->getTcpInfoString(buf, sizeof buf);
    return buf;
}


/*
 * 当有可读事件发生，执行handleRead()回调。
 * 尝试从socketfd中读取数据保存到Buffer中。
 * 若读取长度大于0，将接收的数据通过messageCallback_传递到上层应用（这里是TcpServer）。
 * 如果长度等于0，说明对端客户端关闭了连接，调用handleClose()进行关闭处理；
 * 如果小于0，调用handleError()进行错误处理。
 */
void TcpConnection::handleRead(Timestamp receiveTime) {
    loop_->assertInLoopThread();
    int saveErrno = 0;
    ssize_t n = inputBuffer_.readFd(channel_->fd(),&saveErrno);
    if(n > 0){
        // 用户提供的处理信息的回调函数（由用户自己提供的函数，比如OnMessage）
        messageCallback_(shared_from_this(), &inputBuffer_, receiveTime);
    }else if(n == 0){
        handleClose();  // 读到0则表示客户端已经关闭
    }else{
        errno = saveErrno;
        LOG_SYSERR << "TcpConnection::handleRead";
        handleError();
    }
}


/*
 * 内核中为sockfd分配的发送缓冲区未满时，socketfd将一直处于可写的状态。
 * 由于muduo采用LT水平触发，需要在发送数据的时候才关注可写事件，否则会造成busy loop。
 *
 * 用户发送的数据（直接发送，但不能一次发送完毕的数据）保存在outputBuffer_中，
 * 由于socketfd缓冲区可写空间不足以一次发送完用户数据，
 * TcpConnetion可能接受多次可读事件、分次分段的地保证数据完整的发送。
 * 当发送完毕后，不再关注socketfd上的可写事件。
 *
 * 这里的错误不需要处理，因为一旦发生错误，handleRead()会读到0字节，继而关闭连接
 */
void TcpConnection::handleWrite() {
    loop_->assertInLoopThread();
    if(channel_->isWriting()){
        ssize_t n = sockets::write(channel_->fd(),
                                   outputBuffer_.peek(),
                                   outputBuffer_.readableBytes());
        if( n > 0){
            outputBuffer_.retrieve(n);
            if(outputBuffer_.readableBytes() == 0){ // 发送完毕
                channel_->disableWriting(); // 不在关注fd的可写事件
                if(writeCompleteCallback_){
                    loop_->queueInLoop(std::bind(writeCompleteCallback_,shared_from_this()));
                }
            }
        }else{
            LOG_SYSERR << "TcpConnection::handleWrite";
        }
    }else{ // 已经不可写，不再发送
        LOG_TRACE << "Connection fd = " << channel_->fd()
                  << " is down, no more writing";
    }
}


/*
 * 关闭事件流程复杂一些。首先channel不再关注任何IO事件，
 * 接着调用connectionCallback_回调通知用户连接已经关闭，
 * 最后调用closeCallback_交由TcpServer处理。
 *
 * TcpServer中处理关闭过程，先将当前连接从 connetion map中移除（使用智能指针，引用计数减1），
 * 再调用TcpConnection::connectDestroyed函数（智能指针保证生命周期延长）。
 */
// 当对端调用shutdown()关闭连接时，本端会收到一个FIN，
// channel的读事件被触发，但inputBuffer_.readFd() 会返回0，然后调用
// handleClose()，处理关闭事件，最后调用TcpServer::removeConnection()。
void TcpConnection::handleClose() {
    loop_->assertInLoopThread();
    LOG_TRACE << "fd = " << channel_->fd() << "state = " << stateToString();
    assert(state_ == kConnected || state_ == kDisconnected);
    //我们不关闭fd，而是将其留给dtor，这样我们就可以很容易地发现泄漏
    setState(kDisconnected); // 设置为已断开状态
    channel_->disableAll(); // channel上不再关注任何事情

    TcpConnectionPtr guardThis(shared_from_this()); // 必须使用智能指针
    connectionCallback_(guardThis);  // 回调用户的连接处理回调函数

    // 必须最后调用，回调TcpServer的函数，TcpConnection的生命期由TcpServer控制
    closeCallback_(guardThis);
}

/*
 * 从Poller中移除当前channel。
 * 中间重复的代码，是为了处理不经由handleClose()直接调用connectDestroyed()的情况。
 * 但是要注意调用connectDestroyed()时，
 * channel_仍然是关注一些IO事件的，可能导致程序退出的一些断言异常。
 */
void TcpConnection::connectDestroyed() {
    loop_->assertInLoopThread();
    if(state_ == kConnected){
        setState(kDisconnected);
        channel_->disableAll();
        connectionCallback_(shared_from_this());
    }
    channel_->remove();
}

/*
 * 提供发送数据的接口，实际调用sendInLoop函数，
 * 保证线程安全（虽然数据会拷贝，但是很容易保证线程安全）。
 * 注意，这里没有考虑this的安全性，是因为race condition在正常情况下很难发生。
 */

void TcpConnection::send(const void *message, int len) {
   send(StringPiece(static_cast<const char*>(message),len));
}

void TcpConnection::send(const StringPiece &message) {
    if(state_ == kConnected){
        if(loop_->isInLoopThread()){
            sendInLoop(message);
        }else{
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                             message.as_string()));
            //std::forward<string>(message)));

        }
    }
}


// FIXME efficiency!!!
void TcpConnection::send(Buffer* buf)
{
    if (state_ == kConnected)
    {
        if (loop_->isInLoopThread())
        {
            sendInLoop(buf->peek(), buf->readableBytes());
            buf->retrieveAll();
        }
        else
        {
            void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::sendInLoop;
            loop_->runInLoop(
                    std::bind(fp,
                              this,     // FIXME
                              buf->retrieveAllAsString()));
            //std::forward<string>(message)));
        }
    }
}

/*
 *sendInLoop()会先尝试直接发送数据，如果一次发送完毕就不会启用WriteCallback；
 * 如果只发送了部分数据，则把剩余的数据放入outputBuffer_，
 * 并开始关注可写事件，之后在handlerWrite()中发送剩余的数据。
 * 如果当前outputBuffer_中已经有待发送的数据，那么就不能先尝试发送，因为这会造成数据乱序。
 */

void TcpConnection::sendInLoop(const StringPiece& message)
{
    sendInLoop(message.data(), message.size());
}


void TcpConnection::sendInLoop(const void* data, size_t len){
    loop_->assertInLoopThread();
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;
    if(state_ == kDisconnected){
        LOG_WARN << "disconnected, give up writing";
        return;
    }
    // 如果当前channel没有写事件发生，并且发送buffer无待发送数据，那么直接发送
    if(!channel_->isWriting() && outputBuffer_.readableBytes()==0){
        nwrote = sockets::write(channel_->fd(),data,len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                loop_->queueInLoop((std::bind(writeCompleteCallback_,shared_from_this())));
            }
        } else{
            nwrote = 0;
            if (errno != EWOULDBLOCK){
                LOG_SYSERR << "TcpConnection::sendInLoop";
                if (errno == EPIPE || errno == ECONNRESET) // FIXME: any others?
                {
                    faultError = true; // 仅记录错误原因
                }
            }
        }
    }

    assert(remaining <= len);

    if(!faultError && remaining > 0){
        size_t  oldLen = outputBuffer_.readableBytes();

        // 如果输出缓冲区的数据已经缓冲区最大长度，那么调用highWaterMarkCallback_---告知越界
        if (oldLen + remaining >= highWaterMark_  && oldLen < highWaterMark_ && highWaterMarkCallback_)
        {
            // 加入待处理事件,等待可以处理
            loop_->queueInLoop(std::bind(highWaterMarkCallback_, shared_from_this(), oldLen + remaining));
        }

        // 把数据添加到输出缓冲区中
        outputBuffer_.append(static_cast<const char*>(data)+nwrote, remaining);
        // 监听channel的可写事件（因为还有数据未发完），
        // 当可写事件被触发，就可以继续发送了，调用的是TcpConnection::handleWrite()
        if(!channel_->isWriting()){
            channel_->enableWriting();
        }
    }
}


void TcpConnection::shutdown()
{
    // FIXME: use compare and swap
    if (state_ == kConnected)
    {
        setState(kDisconnecting);
        // FIXME: shared_from_this()?
        loop_->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop()
{
    loop_->assertInLoopThread();
    if (!channel_->isWriting())
    {
        // we are not writing
        socket_->shutdownWrite();
    }
}

void TcpConnection::forceClose()
{
    // FIXME: use compare and swap
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->queueInLoop(std::bind(&TcpConnection::forceCloseInLoop, shared_from_this()));
    }
}

void TcpConnection::forceCloseWithDelay(double seconds)
{
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        setState(kDisconnecting);
        loop_->runAfter(
                seconds,
                makeWeakCallback(shared_from_this(),
                                 &TcpConnection::forceClose));  // not forceCloseInLoop to avoid race condition
    }
}

void TcpConnection::forceCloseInLoop()
{
    loop_->assertInLoopThread();
    if (state_ == kConnected || state_ == kDisconnecting)
    {
        // as if we received 0 byte in handleRead();
        handleClose();
    }
}


const char* TcpConnection::stateToString() const
{
    switch (state_)
    {
        case kDisconnected:
            return "kDisconnected";
        case kConnecting:
            return "kConnecting";
        case kConnected:
            return "kConnected";
        case kDisconnecting:
            return "kDisconnecting";
        default:
            return "unknown state";
    }
}


void TcpConnection::setTcpNoDelay(bool on)
{
    socket_->setTcpNoDelay(on);
}

void TcpConnection::startRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::startReadInLoop, this));
}

void TcpConnection::startReadInLoop()
{
    loop_->assertInLoopThread();
    if (!reading_ || !channel_->isReading())
    {
        channel_->enableReading();
        reading_ = true;
    }
}

void TcpConnection::stopRead()
{
    loop_->runInLoop(std::bind(&TcpConnection::stopReadInLoop, this));
}

void TcpConnection::stopReadInLoop()
{
    loop_->assertInLoopThread();
    if (reading_ || channel_->isReading())
    {
        channel_->disableReading();
        reading_ = false;
    }
}

void TcpConnection::connectEstablished()
{
    loop_->assertInLoopThread();
    assert(state_ == kConnecting);
    setState(kConnected);
    channel_->tie(shared_from_this());
    channel_->enableReading();

    connectionCallback_(shared_from_this());
}


void TcpConnection::handleError()
{
    int err = sockets::getSocketError(channel_->fd());
    LOG_ERROR << "TcpConnection::handleError [" << name_
              << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

