//
// Created by ftion on 2023/6/15.
//

#ifndef MUDUO_NET_TCPCLIENT_H
#define MUDUO_NET_TCPCLIENT_H

#include "../base/Mutex.h"
#include "TcpConnection.h"

namespace muduo{
    namespace net{
        class Connector;
        typedef std::shared_ptr<Connector> ConnectorPtr;

        class TcpClient:noncopyable{
        public:
            TcpClient(EventLoop* loop,
                      const InetAddress& serverAddr,
                      const string& nameArg);
            ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

            void connect();
            void disconnect();
            void stop();

            TcpConnectionPtr connection() const{
                MutexLockGuard lock(mutex_);
                return connection_;
            }

            EventLoop* getLoop() const { return loop_; }
            bool retry() const { return retry_; }
            void enableRetry() { retry_ = true; }

            const string& name() const
            { return name_; }

            /// Set connection callback.
            /// Not thread safe.
            void setConnectionCallback(ConnectionCallback cb)
            { connectionCallback_ = std::move(cb); }

            /// Set message callback.
            /// Not thread safe.
            void setMessageCallback(MessageCallback cb)
            { messageCallback_ = std::move(cb); }

            /// Set write complete callback.
            /// Not thread safe.
            void setWriteCompleteCallback(WriteCompleteCallback cb)
            { writeCompleteCallback_ = std::move(cb); }


        private:
            /// Not thread safe, but in loop
            void newConnection(int sockfd);
            /// Not thread safe, but in loop
            void removeConnection(const TcpConnectionPtr& conn);


            EventLoop* loop_;  // 所属的EventLoop
            ConnectorPtr connector_; 	// 使用Connector智能指针，避免头文件引入
            const string name_;		// 连接的名字

            ConnectionCallback connectionCallback_;		// 建立连接的回调函数
            MessageCallback messageCallback_;				// 消息到来的回调函数
            WriteCompleteCallback writeCompleteCallback_; // 数据发送完毕回调函数
            bool retry_;   // atomic		// 连接断开后是否重连
            bool connect_; // atomic
            // always in loop thread
            int nextConnId_;				// name_+nextConnId_ 用于标识一个连接
            mutable MutexLock mutex_;
            TcpConnectionPtr connection_ GUARDED_BY(mutex_);

        };
    }
}

#endif //MUDUO_NET_TCPCLIENT_H
