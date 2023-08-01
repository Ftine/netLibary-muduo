//
// Created by ftion on 2023/6/15.
//

#ifndef MUDUO_NET_CONNECTOR_H
#define MUDUO_NET_CONNECTOR_H

#include "../base/noncopyable.h"
#include "InetAddress.h"

#include <functional>
#include <memory>

/*
 *std::enable_shared_from_this<Connector> 类是为了解决在使用 std::shared_ptr 进行对象拷贝时，
 * 如果需要在对象内部获取自身的 shared_ptr，而不是新建一个 shared_ptr 拷贝，可以通过继承该类来实现。
 * 在继承 std::enable_shared_from_this<T> 的类中，
 * 可以通过成员函数 shared_from_this() 来获取指向当前对象的 std::shared_ptr<T>。
 */


namespace muduo{
    namespace net{
        class Channel;
        class EventLoop;
        class Connector : noncopyable,
                          public std::enable_shared_from_this<Connector>{
        public:
            typedef std::function<void (int sockfd)> NewConnectionCallback;

            Connector(EventLoop* loop, const InetAddress& serverAddr);  // 构造函数
            ~Connector();

            void setNewConnectionCallback(const NewConnectionCallback& cb)
            { newConnectionCallback_ = cb; }

            void start();       // 安全，实际调用EventLoop::queueInLoop()
            void restart();     // 不安全，只能在lopp thread调用
            void stop();        // 安全，实际调用EventLoop::queueInLoop()

            const InetAddress& serverAddress() const { return serverAddr_; }

        private:
            enum States { kDisconnected, kConnecting, kConnected };
            static const int kMaxRetryDelayMs = 30*1000;  // 重连的最大延迟时间
            static const int kInitRetryDelayMs = 500;     // 初始重连的延迟时间

            void setState(States s) { state_ = s; }
            void startInLoop();
            void stopInLoop();
            void connect();
            void connecting(int sockfd);
            void handleWrite();
            void handleError();
            void retry(int sockfd);
            int removeAndResetChannel();
            void resetChannel();

            EventLoop* loop_;				// 所属的事件循环loop
            InetAddress serverAddr_;     	// 要连接的服务端地址
            bool connect_; // atomic     	// 是否要连接的标志
            States state_;  // FIXME: use atomic variable
            std::unique_ptr<Channel> channel_;			    // 客户端用于通信的socketfd创建的channel
            NewConnectionCallback newConnectionCallback_;   // 建立成功时的回调函数
            int retryDelayMs_;  							// 重连的延迟时间ms
        };
    }
}

#endif //MUDUO_NET_CONNECTOR_H
