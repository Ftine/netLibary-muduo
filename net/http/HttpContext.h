//
// Created by fight on 2023/6/24.
//

#ifndef MUDUO_NET_HTTP_HTTPCONTEXT_H
#define MUDUO_NET_HTTP_HTTPCONTEXT_H

#include "../../base/copyable.h"
#include "HttpRequest.h"

namespace muduo{
    namespace net{
        class Buffer;

        class HttpContext : public muduo::copyable{
        public:
            enum HttpRequestParseState
            {
                kExpectRequestLine,		// 请求行
                kExpectHeaders,			// 请求头
                kExpectBody,		    // 请求体
                kGotAll,
            };
            // 构造函数，默认从请求行开始解析
            HttpContext() : state_(kExpectRequestLine) { }

            // return false if any error
            bool parseRequest(Buffer* buf, Timestamp receiveTime); // 解析请求Buffer

            bool gotAll() const { return state_ == kGotAll; }

            void reset()  // 为了复用HttpContext
            {
                state_ = kExpectRequestLine;
                HttpRequest dummy;
                request_.swap(dummy);
            }

            const HttpRequest& request() const { return request_; }
            HttpRequest& request() { return request_; }


        private:
            bool processRequestLine(const char* begin, const char* end);

            HttpRequestParseState state_; // 解析状态
            HttpRequest request_;

        };
    }
}

#endif //MUDUO_NET_HTTP_HTTPCONTEXT_H
