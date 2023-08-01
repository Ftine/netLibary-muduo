//
// Created by fight on 2023/6/11.
//

#ifndef MUDUO_NET_TCPCONNECTION_H
#define MUDUO_NET_TCPCONNECTION_H

#include "../base/noncopyable.h"
#include "../base/StringPiece.h"
#include "../base/Types.h"
#include "Callbacks.h"
#include "Buffer.h"
#include "InetAddress.h"

#include <memory>
#include <boost/any.hpp>

struct tcp_info;

namespace muduo{
    namespace net{
        class Channel;
        class EventLoop;
        class Socket;

        class TcpConnection : noncopyable,
                              public std::enable_shared_from_this<TcpConnection>{

        public:
            TcpConnection(EventLoop* loop,const string& name,
                          int sockfd,const InetAddress& localAddr,const InetAddress& peerAddr);
            ~TcpConnection();

            // 获取当前TcpConnetction的一些属性、状态
            EventLoop* getLoop() const { return loop_; }
            const string& name() const { return name_; }
            const InetAddress& localAddress() const { return localAddr_; }
            const InetAddress& peerAddress() const { return peerAddr_; }
            bool connected() const { return state_ == kConnected; }
            bool disconnected() const { return state_ == kDisconnected; }
            // return true if success.
            bool getTcpInfo(struct tcp_info*) const;
            string getTcpInfoString() const;

            /// 提供上层调用的发送接口，直接发送或保存在Buffer中
            // void send(string&& message); // C++11
            void send(const void* message, int len);
            void send(const StringPiece& message);
            // void send(Buffer&& message); // C++11
            void send(Buffer* message);  // this one will swap data

            // 关闭连接，设置TCP选项
            void shutdown(); // NOT thread safe, no simultaneous calling
            // void shutdownAndForceCloseAfter(double seconds); // NOT thread safe, no simultaneous calling
            void forceClose();
            void forceCloseWithDelay(double seconds);
            void setTcpNoDelay(bool on);

            // 开启/关闭当前连接socket上的可读事件监听（通过channel传递到Poller）
            void startRead();
            void stopRead();
            bool isReading() const { return reading_; }; // NOT thread safe, may race with start/stopReadInLoop

            void setContext(const boost::any& context)
            { context_ = context; }

            const boost::any& getContext() const
            { return context_; }

            boost::any* getMutableContext()
            { return &context_; }

            // 设置回调函数
            void setConnectionCallback(const ConnectionCallback& cb)
            { connectionCallback_ = cb; }

            void setMessageCallback(const MessageCallback& cb)
            { messageCallback_ = cb; }

            void setWriteCompleteCallback(const WriteCompleteCallback& cb)
            { writeCompleteCallback_ = cb; }
            // 限制发送缓冲区最大的数据堆积量，一旦超过设定值调用指定回调函数
            void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
            { highWaterMarkCallback_ = cb; highWaterMark_ = highWaterMark; }

            /// Advanced interface 输入输出缓冲区，应用层不用关系底层处理流程
            Buffer* inputBuffer()
            { return &inputBuffer_; }
            Buffer* outputBuffer()
            { return &outputBuffer_; }

            /// Internal use only.
            void setCloseCallback(const CloseCallback& cb)
            { closeCallback_ = cb; }

            // called when TcpServer accepts a new connection
            void connectEstablished();   // should be called only once
            // called when TcpServer has removed me from its map
            void connectDestroyed();  // should be called only once

        private:
            enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

            void handleRead(Timestamp receiveTime);
            void handleWrite();
            void handleClose();
            void handleError();
            // void sendInLoop(string&& message);
            void sendInLoop(const StringPiece& message);
            void sendInLoop(const void* message, size_t len);
            void shutdownInLoop();
            // void shutdownAndForceCloseInLoop(double seconds);
            void forceCloseInLoop();
            void setState(StateE s) { state_ = s; }
            const char* stateToString() const;
            void startReadInLoop();    // 开始接收可读事件
            void stopReadInLoop();




            EventLoop* loop_;
            const string name_;
            StateE state_;  // FIXME: use atomic variable
            bool reading_;
            // 已连接的socketfd的封装Socket、Channel对象
            std::unique_ptr<Socket> socket_;
            std::unique_ptr<Channel> channel_;

            const InetAddress localAddr_;
            const InetAddress peerAddr_;

            // IO事件回调，要求在EventLoop中执行
            ConnectionCallback connectionCallback_;
            MessageCallback messageCallback_;
            WriteCompleteCallback writeCompleteCallback_;
            HighWaterMarkCallback highWaterMarkCallback_;
            CloseCallback closeCallback_;

            // 底层的输入、输出缓冲区的处理
            size_t highWaterMark_;
            Buffer inputBuffer_;
            Buffer outputBuffer_; // FIXME: use list<Buffer> as output buffer.
            boost::any context_;
            // FIXME: creationTime_, lastReceiveTime_
            //        bytesReceived_, bytesSent_
        };

        typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
    }
}

#endif //MUDUO_NET_TCPCONNECTION_H
