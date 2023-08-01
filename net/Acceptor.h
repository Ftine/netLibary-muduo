//
// Created by fight on 2023/6/11.
//

#ifndef MUDUO_NET_ACCEPTOR_H
#define MUDUO_NET_ACCEPTOR_H
#include "Socket.h"
#include "Channel.h"

#include <functional>


namespace muduo{
    namespace net{
        class EventLoop;
        class InetAddress;

        class Acceptor : noncopyable{
        public:
            typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

            Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
            ~Acceptor();

            void setNewConnectionCallback(const NewConnectionCallback& cb)
            { newConnectionCallback_ = cb; }

            bool listenning() const { return listenning_; }
            void listen();

        private:

            // 当有客户端发起连接时，监听channel触发读事件，
            // 那么调用其对应的回调函数Acceptor::handleRead()。
            void handleRead();

            EventLoop* loop_;  // 基本是main reactor
            Socket acceptSocket_; // 用于接收新连接的scoket封装
            Channel acceptChannel_;  // 封装acceptSocket_的channel，监听其上的事件
            NewConnectionCallback newConnectionCallback_; // 建立新连接时调用的回调函数
            bool listenning_;

            // 一个文件描述符号，用于占用一个空闲的文件描述符号，避免在调用 accept() 函数时返回一个非预期的文件描述符号。
            // idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC)
            int idleFd_;
        };
    }
}

#endif //MUDUO_NET_ACCEPTOR_H
