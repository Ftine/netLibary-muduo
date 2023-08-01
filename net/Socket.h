//
// Created by fight on 2023/6/1.
//

#ifndef MUDUO_NET_SOCKET_H
#define MUDUO_NET_SOCKET_H

#include "../base/noncopyable.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo{
    namespace net{
        class InetAddress;

        class Socket : noncopyable{
        public:
            explicit Socket(int sockfd) : sockfd_(sockfd){}
            ~Socket();

            int fd() const { return sockfd_;}

            // 获取TCP的信息
            bool getTcpInfo(struct tcp_info *) const;
            bool getTcpInfoString(char* buf, int len) const;

            // 绑定地址
            void bindAddress(const InetAddress& localaddr);
            // 监听地址
            void listen();

            /*
             * 成功时，返回一个非负整数 - 已接受的套接字的描述符
             * 设置为非阻塞和关闭执行。 *peeraddr 已分配。
             * 出错时，返回 -1，*peeraddr 保持不变。
             */
            int accept(InetAddress* peeraddr);

            // 关闭写
            void shutdownWrite();


            // 启用/禁用 TCP_NODELAY（禁用/启用 Nagle 算法）。
            void setTcpNoDelay(bool on);


            // 启用/禁用 SO_REUSEADDR
            void setReuseAddr(bool on);

            // Enable/disable SO_REUSEPORT
            void setReusePort(bool on);


            // Enable/disable SO_KEEPALIVE
            void setKeepAlive(bool on);

        private:
            const int sockfd_;
        };
    }
}

#endif //MUDUO_NET_SOCKET_H
