//
// Created by fight on 2023/6/2.
//

#ifndef MUDUO_NET_INETADDRESS_H
#define MUDUO_NET_INETADDRESS_H

#include "../base/copyable.h"
#include "../base/StringPiece.h"

#include <netinet/in.h>

namespace muduo{
    namespace net{
        namespace sockets{
            const struct sockaddr* sockaddr_cast(const struct sockaddr_in6* addr);
        }

    class InetAddress : public muduo::copyable{
    public:
        // 构造具有给定端口号的端点。
        // 多用于TcpServer监听。
        explicit InetAddress(uint16_t port = 0, bool loopbackOnly = false, bool ipv6 = false);

        // 使用给定的 ip 和端口构造一个端点。
        // @c ip 应该是“1.2.3.4”
        InetAddress(StringArg ip, uint16_t port, bool ipv6 = false);

        // 用给定的结构 @c sockaddr_in 构造一个端点
        // 大多在接受新连接时使用 - ipv4
        explicit InetAddress(const struct sockaddr_in& addr)
                : addr_(addr)
        { }

        // 在接受新连接时使用 - ipv6
        explicit InetAddress(const struct sockaddr_in6& addr)
                : addr6_(addr)
        { }

        sa_family_t family() const { return addr_.sin_family; }
        string toIp() const;
        string toIpPort() const;
        uint16_t toPort() const;

        const struct sockaddr* getSockAddr() const { return sockets::sockaddr_cast(&addr6_); }
        void setSockAddrInet6(const struct sockaddr_in6& addr6) { addr6_ = addr6; }

        uint32_t ipNetEndian() const;
        uint16_t portNetEndian() const { return addr_.sin_port; }

        // 将主机名解析为 IP 地址，不更改端口或 sin_family
        // 成功返回真。- 线程安全
        static bool resolve(StringArg hostname, InetAddress* result);

        // 设置 IPv6 范围 ID
        void setScopeId(uint32_t scope_id);

    private:
        union
        {
            struct sockaddr_in addr_;
            struct sockaddr_in6 addr6_;
        };

        };
    }
}

#endif //MUDUO_NET_INETADDRESS_H
