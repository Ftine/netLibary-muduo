//
// Created by fight on 2023/6/24.
//

#ifndef MUDUO_NET_HTTP_HTTPSERVER_H
#define MUDUO_NET_HTTP_HTTPSERVER_H
#include "../TcpServer.h"

namespace muduo{
    namespace net{
        class HttpRequest;
        class HttpResponse;

        class HttpServer : noncopyable{
        public:
            typedef std::function<void (const HttpRequest&, HttpResponse*)> HttpCallback;

            HttpServer(EventLoop* loop,
                       const InetAddress& listenAddr,
                       const string& name,
                       TcpServer::Option option = TcpServer::kNoReusePort);

            EventLoop* getLoop() const { return server_.getLoop(); }

            /// Not thread safe, callback be registered before calling start().
            // 设置http请求的回调函数
            void setHttpCallback(const HttpCallback& cb) { httpCallback_ = cb; }

            void setThreadNum(int numThreads) { server_.setThreadNum(numThreads); }

            void start();


        private:
            // TcpServer的新连接、新消息的回调函数
            void onConnection(const TcpConnectionPtr& conn);
            void onMessage(const TcpConnectionPtr& conn, Buffer* buf,Timestamp receiveTime);

            // 在onMessage中调用，并调用用户注册的httpCallback_函数，对请求进行具体的处理
            void onRequest(const TcpConnectionPtr&, const HttpRequest&);


            TcpServer server_;
            HttpCallback httpCallback_;


        };

    }
}

#endif //MUDUO_NET_HTTP_HTTPSERVER_H
