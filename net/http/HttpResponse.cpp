//
// Created by ftion on 2023/6/20.
//


#include "HttpResponse.h"
#include "../Buffer.h"

#include <stdio.h>

using namespace muduo;
using namespace muduo::net;

void HttpResponse::appendToBuffer(Buffer* output) const
{
    char buf[32];
    // 响应行
    snprintf(buf, sizeof buf, "HTTP/1.1 %d ", statusCode_);
    output->append(buf);
    output->append(statusMessage_);
    output->append("\r\n");

    // 响应头部
    if (closeConnection_){
        output->append("Connection: close\r\n");
    } else{
        snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
        output->append(buf);
        output->append("Connection: Keep-Alive\r\n");
    }

    for (const auto& header : headers_){
        output->append(header.first);
        output->append(": ");
        output->append(header.second);
        output->append("\r\n");
    }

    output->append("\r\n");  // 空行
    output->append(body_);   // 响应体
}
