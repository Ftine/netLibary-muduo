//
// Created by fight on 2023/6/24.
//

#include "HttpContext.h"
#include "../Buffer.h"

using namespace muduo;
using namespace muduo::net;

bool HttpContext::parseRequest(Buffer* buf, Timestamp receiveTime)
{
    bool ok = true;
    bool hasMore = true;
    while (hasMore)
    {
        if (state_ == kExpectRequestLine)  // 解析请求行
        { // 查找Buffer中第一次出现“\r\n”的位置
            const char* crlf = buf->findCRLF();
            if (crlf)
            { // 若找到“\r\n”，说明至少有一行数据，进行实际解析
                // buf->peek()为数据开始部分，crlf为结束部分
                ok = processRequestLine(buf->peek(), crlf);
                if (ok){ // 解析成功
                    request_.setReceiveTime(receiveTime);
                    buf->retrieveUntil(crlf + 2); // buffer->peek()向后移2字节，到下一行
                    state_ = kExpectHeaders;
                }
                else { hasMore = false;}
            }
            else {  hasMore = false; }
        }else if (state_ == kExpectHeaders)  // 解析请求头
        {
            const char* crlf = buf->findCRLF(); // 找到“\r\n”位置
            if (crlf)
            {
                const char* colon = std::find(buf->peek(), crlf, ':'); // 定位分隔符
                if (colon != crlf){
                    request_.addHeader(buf->peek(), colon, crlf); // 键值对解析
                }
                else{
                    // empty line, end of header
                    // FIXME:
                    state_ = kGotAll;
                    hasMore = false;
                }
                buf->retrieveUntil(crlf + 2); // 后移2个字符
            }
            else{ hasMore = false; }
        }
        else if (state_ == kExpectBody)  // 解析请求体，未实现
        {
            // FIXME:
        }
    }
    return ok;
}


bool HttpContext::processRequestLine(const char* begin, const char* end)
{
    bool succeed = false;
    const char* start = begin;
    const char* space = std::find(start, end, ' ');
    // 第一个空格前的字符串，请求方法
    if (space != end && request_.setMethod(start, space))
    {
        start = space+1;
        space = std::find(start, end, ' ');
        if (space != end)
        {  // 第二个空格前的字符串，URL
            const char* question = std::find(start, space, '?');
            if (question != space){  // 如果有"?"，分割成path和请求参数
                request_.setPath(start, question);
                request_.setQuery(question, space);
            }
            else{
                request_.setPath(start, space); // 仅path
            }
            start = space+1;
            // 最后一部分，解析HTTP协议
            succeed = end-start == 8 && std::equal(start, end-1, "HTTP/1.");
            if (succeed)
            {
                if (*(end-1) == '1'){  // 1.1版本
                    request_.setVersion(HttpRequest::kHttp11);
                }
                else if (*(end-1) == '0'){  // 1.0版本
                    request_.setVersion(HttpRequest::kHttp10);
                }
                else{
                    succeed = false;
                }
            }
        }
    }
    return succeed;
}




