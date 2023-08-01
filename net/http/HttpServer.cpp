//
// Created by fight on 2023/6/24.
//

#include "HttpServer.h"

#include "../../base/Logging.h"
#include "HttpContext.h"
#include "HttpRequest.h"
#include "HttpResponse.h"

using namespace muduo;
using namespace muduo::net;

namespace muduo {
    namespace net {
        namespace detail {

            void defaultHttpCallback(const HttpRequest &, HttpResponse *resp) {
                resp->setStatusCode(HttpResponse::k404NotFound);
                resp->setStatusMessage("Not Found");
                resp->setCloseConnection(true);
            }

        }  // namespace detail
    }  // namespace net
}  // namespace muduo


HttpServer::HttpServer(muduo::net::EventLoop *loop, const muduo::net::InetAddress &listenAddr, const std::string &name,
                       TcpServer::Option option) : server_(loop, listenAddr, name, option) {
    server_.setConnectionCallback(
            std::bind(&HttpServer::onConnection, this, _1));
    server_.setMessageCallback(
            std::bind(&HttpServer::onMessage, this, _1, _2, _3));

}

void HttpServer::start()
{
    LOG_WARN << "HttpServer[" << server_.name()
             << "] starts listening on " << server_.ipPort();
    server_.start();
}

void HttpServer::onConnection(const TcpConnectionPtr& conn)
{
    if (conn->connected())
    {
        conn->setContext(HttpContext());
    }
}


void HttpServer::onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp receiveTime)
{
    HttpContext* context = boost::any_cast<HttpContext>(conn->getMutableContext());
    // 解析请求
    if (!context->parseRequest(buf, receiveTime)){
        conn->send("HTTP/1.1 400 Bad Request\r\n\r\n");  // 解析失败， 400错误
        conn->shutdown();   // 断开本段写
    }
    // 请求解析成功
    if (context->gotAll()){
        onRequest(conn, context->request());    // 调用onRequest()私有函数
        context->reset();					        // 复用HttpContext对象
    }
}


void HttpServer::onRequest(const TcpConnectionPtr& conn, const HttpRequest& req)
{
    // 长连接还是短连接
    const string& connection = req.getHeader("Connection");
    bool close = connection == "close" ||
                 (req.getVersion() == HttpRequest::kHttp10 && connection != "Keep-Alive");

    HttpResponse response(close);
    httpCallback_(req, &response);  // 调用客户端的http处理函数，填充response
    Buffer buf;
    response.appendToBuffer(&buf); // 将响应格式化填充到buf中
    conn->send(&buf);				 // 将响应回复发送给客户端
    if (response.closeConnection()){
        conn->shutdown();  // 如果是短连接，直接关闭。
    }
}






