//
// Created by tcy on 2024/5/18.
//

#ifndef RTSP_TCP_SOCKET_H
#define RTSP_TCP_SOCKET_H

#include "inet_address.h"
#include "socket_ops.h"

class TcpSocket {
public:
    explicit TcpSocket(int32_t sockfd) :m_sockfd_(sockfd) {}
    ~TcpSocket() { Sockets::Close(m_sockfd_); }

    int32_t GetSockFd() const { return m_sockfd_; }
    bool Bind(Ipv4Address& addr) {
        return Sockets::Bind(m_sockfd_, addr.GetIp(), addr.GetPort());
    }
    bool Listen(int32_t backlog) {
        return Sockets::Listen(m_sockfd_, backlog);
    }
    int32_t Accept() {
        return Sockets::Accept(m_sockfd_);
    }
    void SetReuseAddr(int32_t on) {
        Sockets::SetReuseAddr(m_sockfd_, on);
    }

private:
    int32_t m_sockfd_;
};

#endif //RTSP_TCP_SOCKET_H
