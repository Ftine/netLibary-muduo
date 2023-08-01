//
// Created by fight on 2023/6/11.
//

#include "Acceptor.h"

#include "../base/Logging.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "SocketsOpts.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

using namespace muduo;
using namespace muduo::net;

Acceptor::Acceptor(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, bool reuseport)
        : loop_(loop),
          acceptSocket_(sockets::createNonblockingOrDie(listenAddr.family())),
          acceptChannel_(loop, acceptSocket_.fd()),
          listenning_(false),
          idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
    assert(idleFd_ >= 0);
    // 设置服务端socket选项，并绑定到指定ip和port
    acceptSocket_.setReuseAddr(true);      // addr重用
    acceptSocket_.setReusePort(reuseport); // 端口重用
    acceptSocket_.bindAddress(listenAddr); // bind
    // 使用channel监听socket上的可读事件（新的连接）
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead, this));

}


Acceptor::~Acceptor()
{
    // 不关注socket上的IO事件，从EventLoop的Poller注销
    acceptChannel_.disableAll();
    acceptChannel_.remove();
    // 关闭socket
    ::close(idleFd_);
}


void Acceptor::listen()
{
    loop_->assertInLoopThread();
    listenning_ = true;
    acceptSocket_.listen();
    acceptChannel_.enableReading();
}


void Acceptor::handleRead() {
    loop_->assertInLoopThread();
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if (connfd >= 0) //新的连接成功
    {
        // string hostport = peerAddr.toIpPort();
        // LOG_TRACE << "Accepts of " << hostport;
        if (newConnectionCallback_){
            // 建立新的连接，调用TcpServer的回调，返回已连接的socketfd和peer端地址
            newConnectionCallback_(connfd, peerAddr);
        }
        else{
            // 若上层应用TcpServer未注册新连接回调函数，则直接关闭当前连接
            sockets::close(connfd);
        }
    }
    else  // 连接异常，处理服务端fd耗尽
    {
        LOG_SYSERR << "in Acceptor::handleRead";
        // Read the section named "The special problem of
        // accept()ing when you can't" in libev's doc.
        // By Marc Lehmann, author of libev.
        if (errno == EMFILE)   // 无可用fd，如不处理，否则Poller水平触发模式下会一直触发
        {
            ::close(idleFd_); // 关闭空闲的fd，空出一个可用的fd
            idleFd_ = ::accept(acceptSocket_.fd(), NULL, NULL); // 把前面调用acceptor没有接受的描述符接受进来到idleFd_
            ::close(idleFd_); // 把这个idleFd_ 关闭，就是关闭了当前此次连接
            idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC); // 重新开启这个空闲描述符
        }
    }
}




