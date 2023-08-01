//
// Created by fight on 2023/6/24.
//

#ifndef MUDUO_NET_HTTP_HTTPREQUEST_H
#define MUDUO_NET_HTTP_HTTPREQUEST_H
#include "../../base/copyable.h"
#include "../../base/Timestamp.h"
#include "../../base/Types.h"

#include <map>
#include <assert.h>
#include <stdio.h>

namespace muduo{
    namespace net{

        class HttpRequest : public muduo::copyable{
        public:
            enum Method  { kInvalid, kGet, kPost, kHead, kPut, kDelete };  //设计支持的请求类型
            enum Version { kUnknown, kHttp10, kHttp11 };  // HTTP版本
            HttpRequest() : method_(kInvalid), version_(kUnknown) {}  //默认构造函数

            void setVersion(Version v) { version_ = v; }      // 版本
            Version getVersion() const { return version_; }

            bool setMethod(const char* start, const char* end)  // 根据字符串设定请求方法
            {
                assert(method_ == kInvalid);
                string m(start, end);
                if      (m == "GET")  {  method_ = kGet;  }
                else if (m == "POST") {  method_ = kPost; }
                else if (m == "HEAD") {  method_ = kHead; }
                else if (m == "PUT")  {  method_ = kPut;  }
                else if (m == "DELETE"){ method_ = kDelete; }
                else                   { method_ = kInvalid;}
                return method_ != kInvalid;
            }
            Method method() const { return method_; }

            const char* methodString() const  // 请类型字符串
            {
                const char* result = "UNKNOWN";
                switch(method_)
                {
                    case kGet:    result = "GET";    break;
                    case kPost:   result = "POST";   break;
                    case kHead:   result = "HEAD";   break;
                    case kPut:    result = "PUT";    break;
                    case kDelete: result = "DELETE"; break;
                    default: break;
                }
                return result;
            }

            // 请求行的URL
            void setPath(const char* start, const char* end) { path_.assign(start, end); }
            const string& path() const { return path_; }
            // 请求体
            void setQuery(const char* start, const char* end) { query_.assign(start, end);  }
            const string& query() const { return query_; }
            // 请求事件的时间
            void setReceiveTime(Timestamp t) { receiveTime_ = t; }
            Timestamp receiveTime() const  { return receiveTime_; }

            // 请求头的添加键值对
            void addHeader(const char* start, const char* colon, const char* end)
            {
                string field(start, colon);  // 要求冒号前无空格
                ++colon;
                while (colon < end && isspace(*colon)) {  // 过滤冒号后的空格
                    ++colon;
                }
                string value(colon, end);
                while (!value.empty() && isspace(value[value.size()-1])){
                    value.resize(value.size()-1);
                }
                headers_[field] = value;
            }

            // 请求头部查找键的值
            string getHeader(const string& field) const
            {
                string result;
                std::map<string, string>::const_iterator it = headers_.find(field);
                if (it != headers_.end()){
                    result = it->second;
                }
                return result;
            }

            const std::map<string, string>& headers() const { return headers_; }

            void swap(HttpRequest& that)  // 交换
            {
                std::swap(method_, that.method_);
                std::swap(version_, that.version_);
                path_.swap(that.path_);
                query_.swap(that.query_);
                receiveTime_.swap(that.receiveTime_);
                headers_.swap(that.headers_);
            }



        private:
            Method method_;						// 请求行 - 请求方法
            Version version_;					// 请求行 - HTTP版本
            string path_;						// 请求行 - URL

            string query_;						// 请求体
            Timestamp receiveTime_;				// 请求事件
            std::map<string, string> headers_;	// 请求头部


        };
    }
}
#endif //MUDUO_NET_HTTP_HTTPREQUEST_H
