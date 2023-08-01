//
// Created by ftion on 2023/6/20.
//

#ifndef MUDUO_NET_HTTP_HTTPRESPONE_H
#define MUDUO_NET_HTTP_HTTPRESPONE_H
#include "../../base/copyable.h"
#include "../../base/Types.h"

#include <map>

namespace muduo{
    namespace net{
        class Buffer;
        class HttpResponse : public muduo::copyable{
        public:
            enum HttpStatusCode{
                kUnknown,
                k200Ok = 200,					// 正常
                k301MovedPermanently = 301,	    // 资源不可访问，重定向
                k400BadRequest = 400,			// 请求错误（域名不存在、请求不正确）
                k404NotFound = 404,				// 通常是URL不正确（或者因为服务不再提供）
            };

            explicit HttpResponse(bool close) : statusCode_(kUnknown), closeConnection_(close){}

            void setStatusCode(HttpStatusCode code) { statusCode_ = code; }

            void setStatusMessage(const string& message) { statusMessage_ = message; }

            void setCloseConnection(bool on) { closeConnection_ = on; }

            void setContentType(const string& contentType) { addHeader("Content-Type", contentType); }

            // FIXME: replace string with StringPiece
            void addHeader(const string& key, const string& value) { headers_[key] = value; }

            void setBody(const string& body) { body_ = body; }

            bool closeConnection() const { return closeConnection_; }

            void appendToBuffer(Buffer* output) const;  // 将整个HttpRespose对象按照协议输出到Buffer中


        private:
            std::map<string, string> headers_;   	// 响应头部，键值对
            HttpStatusCode statusCode_;		   	// 响应行 - 状态码
            // FIXME: add http version
            string statusMessage_;				// 响应行 - 状态码文字描述
            bool closeConnection_;				// 是否关闭连接
            string body_;  						// 响应体

        };
    }
}



#endif //MUDUO_NET_HTTP_HTTPRESPONE_H
