//
// Created by fight on 2023/6/1.
//

#include "Socket.h"
#include "../base/Logging.h"
#include "InetAddress.h"
#include "SocketsOpts.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>


using namespace muduo;
using namespace muduo::net;

Socket::~Socket() {
    sockets::close(sockfd_);
}



/**
* 获取TcpInfo(Tcp信息), 存放到tcp_info结构对象tcpi中.
* @details 调用getsockopt获取TcpInfo, 对应opt name为TCP_INFO.
* @param tcpi [in] 指向tcp_info结构, 用来存放TcpInfo
* @return 获取TcpInfo结果
* - true 获取成功
* - false 获取失败
*/
bool Socket::getTcpInfo(struct tcp_info *tcpi) const
{
    socklen_t len = sizeof(*tcpi);
    memZero(tcpi, len);
    return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

/**
* 将从getTcpInfo得到的TcpInfo转换为字符串, 存放到长度为len的buf中.
* @param buf 存放TcpInfo字符串的缓存
* @param len 缓存buf的长度, 单位是字节数
* @return 调用结果
* - true 返回存放到buf的TcpInfo有效
* - false 调用getsockopt()出错
* @see
* https://blog.csdn.net/zhangskd/article/details/8561950
*/
bool Socket::getTcpInfoString(char* buf, int len) const
{
    struct tcp_info tcpi;
    bool ok = getTcpInfo(&tcpi);
    if (ok)
    {
        snprintf(buf, static_cast<size_t>(len), "unrecovered=%u "
                                                "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
                                                "lost=%u retrans=%u rtt=%u rttvar=%u "
                                                "sshthresh=%u cwnd=%u total_retrans=%u",
                 tcpi.tcpi_retransmits,  // 重传数, 当前待重传的包数, 重传完成后清零 //　Number of unrecovered [RTO] timeouts
                 tcpi.tcpi_rto,          // 重传超时时间(usec)　// Retransmit timeout in usec
                 tcpi.tcpi_ato,          // 延时确认的估值(usec) // Predicted tick of soft clock in usec
                 tcpi.tcpi_snd_mss,      // 发送端MSS(最大报文段长度)
                 tcpi.tcpi_rcv_mss,      // 接收端MSS(最大报文段长度)
                 tcpi.tcpi_lost,         // 丢失且未恢复的数据段数 // Lost packets
                 tcpi.tcpi_retrans,      // 重传且未确认的数据段数 // Retransmitted packets out
                 tcpi.tcpi_rtt,          // 平滑的RTT(usec)　//　Smoothed round trip time in usec
                 tcpi.tcpi_rttvar,       // 1/4 mdev (usec) // Medium deviation
                 tcpi.tcpi_snd_ssthresh, // 慢启动阈值
                 tcpi.tcpi_snd_cwnd,     // 拥塞窗口
                 tcpi.tcpi_total_retrans // 本连接的总重传个数  // Total retransmits for entire connection
        );
    }
    return ok;
}

void Socket::bindAddress(const InetAddress& addr)
{
    sockets::bindOrDie(sockfd_, addr.getSockAddr());
}

void Socket::listen()
{
    sockets::listenOrDie(sockfd_);
}

int Socket::accept(InetAddress* peeraddr)
{
    struct sockaddr_in6 addr;
    memZero(&addr, sizeof addr);
    int connfd = sockets::accept(sockfd_, &addr);
    if (connfd >= 0)
    {
        peeraddr->setSockAddrInet6(addr);
    }
    return connfd;
}

void Socket::shutdownWrite()
{
    sockets::shutdownWrite(sockfd_);
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}

void Socket::setReusePort(bool on)
{
#ifdef SO_REUSEPORT
    int optval = on ? 1 : 0;
    int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                           &optval, static_cast<socklen_t>(sizeof optval));
    if (ret < 0 && on)
    {
        LOG_SYSERR << "SO_REUSEPORT failed.";
    }
#else
    if (on)
  {
    LOG_ERROR << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
                 &optval, static_cast<socklen_t>(sizeof optval));
    // FIXME CHECK
}
