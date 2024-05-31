//
// Created by tcy on 2024/5/17.
//

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include "../../include/net/socket_ops.h"
#include "../../include/base/log.h"

int32_t Sockets::CreateTcpSock() {
    return ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
}

int32_t Sockets::CreateUdpSock() {
    return ::socket(AF_INET, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, 0);
}

bool Sockets::Bind(int32_t sockfd, const std::string& ip, uint16_t port) {
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);

    if(::bind(sockfd, (sockaddr*)&addr, sizeof(addr)) < 0) {
        LOG_ERROR("bind error!\n");
        return false;
    }
    return true;
}

bool Sockets::Listen(int32_t sockfd, int32_t backlog) {
    if(::listen(sockfd, backlog) < 0) {
        LOG_ERROR("listen error!\n");
        return false;
    }
    return true;
}

int32_t Sockets::Accept(int32_t sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr_in);

    auto connfd = ::accept(sockfd, (sockaddr*)&addr, &addr_len);
    SetNonBlockAndCloseOnExec(connfd);
    IgnoreSigPipeOnSocket(connfd);
    return connfd;
}

int32_t Sockets::Readv(int32_t sockfd, const struct iovec *iov, int32_t iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

int32_t Sockets::Write(int32_t sockfd, const void *buf, int32_t size) {
    return ::write(sockfd, buf,size);
}

int32_t Sockets::Sendto(int32_t sockfd, const void *buf, int32_t len, const struct sockaddr *dest_addr) {
    socklen_t addr_len = sizeof(struct sockaddr);
    return ::sendto(sockfd, buf, len, 0, dest_addr, addr_len);
}

void Sockets::SetNonBlock(int32_t sockfd) {
    auto flags = ::fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags | O_NONBLOCK);
}

void Sockets::SetBlock(int32_t sockfd, int32_t write_timeout) {
    auto flags = ::fcntl(sockfd, F_GETFL, 0);
    fcntl(sockfd, F_SETFL, flags & (~O_NONBLOCK));

    if(write_timeout > 0) {
        struct timeval tv = {write_timeout / 1000, write_timeout % 1000 * 1000};
        ::setsockopt(sockfd, SOL_SOCKET, SO_SNDTIMEO, (char*)&tv, sizeof(tv));
    }
}

void Sockets::SetReuseAddr(int32_t sockfd, int32_t on) {
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&optval, sizeof(optval));
}

void Sockets::SetReusePort(int32_t sockfd) {
#ifdef SO_REUSEPORT
    int on = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (const char*)&on, sizeof(on));
#endif
}

void Sockets::SetNonBlockAndCloseOnExec(int32_t sockfd) {
    auto flags = ::fcntl(sockfd, F_GETFL, 0);
    flags |= O_NONBLOCK;
    auto ret = ::fcntl(sockfd, F_SETFL, flags);

    flags = ::fcntl(sockfd, F_GETFD, 0);
    flags |= FD_CLOEXEC;
    flags = ::fcntl(sockfd, F_SETFD, flags);
}

void Sockets::IgnoreSigPipeOnSocket(int32_t socketfd) {
    int option = 1;
    ::setsockopt(socketfd, SOL_SOCKET, MSG_NOSIGNAL, &option, sizeof(option));
}

void Sockets::SetNoDelay(int32_t sockfd) {
#ifdef TCP_NODELAY
    int on = 1;
    ::setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char*)&on, sizeof(on));
#endif
}

void Sockets::SetKeepAlive(int32_t sockfd) {
    int on = 1;
    ::setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char*)&on, sizeof(on));
}

void Sockets::SetNoSigpipe(int32_t sockfd) {

}

void Sockets::SetSendBufSize(int32_t sockfd, int32_t size) {
    ::setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *)&size, sizeof(size));
}

void Sockets::SetRecvBufSize(int sockfd, int size) {
    ::setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(size));
}

std::string Sockets::GetPeerIp(int32_t sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(struct sockaddr_in);
    if(::getpeername(sockfd, (sockaddr*)&addr, &addr_len) == 0) {
        return inet_ntoa(addr.sin_addr);
    }
    return "0.0.0.0";
}

int16_t Sockets::GetPeerPort(int32_t sockfd) {
    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(sockaddr_in);
    if(::getpeername(sockfd, (sockaddr*)&addr, &addr_len) == 0) {
        return ntohs(addr.sin_port);
    }
    return 0;
}

int32_t Sockets::GetPeerAddr(int sockfd, struct sockaddr_in *addr) {
    socklen_t addrlen = sizeof(struct sockaddr_in);
    return getpeername(sockfd, (struct sockaddr *)addr, &addrlen);
}

void Sockets::Close(int sockfd) {
    ::close(sockfd);
}

bool Sockets::Connect(int32_t sockfd, const std::string& ip, uint16_t port, int32_t timeout) {
    bool is_connected = true;
    if(timeout > 0) {
        SetNonBlock(sockfd);
    }

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    addr.sin_port = htons(port);
    if(::connect(sockfd, (sockaddr*)&addr, addr_len) < 0) {
        if(timeout > 0) {
            is_connected = false;
            fd_set fd_write;
            FD_ZERO(&fd_write);
            FD_SET(sockfd, &fd_write);
            struct timeval tv = {timeout / 1000, timeout % 1000 * 1000};
            select(sockfd + 1, nullptr, &fd_write, nullptr, &tv);
            if(FD_ISSET(sockfd, &fd_write)) {
                is_connected = true;
            }
            SetBlock(sockfd, 0);
        } else {
            is_connected = false;
        }
    }
    return is_connected;
}

std::string Sockets::GetLocalIp() {
    char buf[512] = {0};
    struct ifconf ifc;
    struct ifreq *ifr;
    auto sockfd = socket(AF_INET,SOCK_DGRAM, 0);
    if(sockfd < 0) {
        close(sockfd);
        return "0.0.0.0";
    }

    ifc.ifc_len = 512;
    ifc.ifc_buf = buf;
    if(ioctl(sockfd, SIOCGIFCONF, &ifc) < 0) {
        close(sockfd);
        return "0.0.0.0";
    }
    ifr = (struct ifreq*)ifc.ifc_ifcu.ifcu_buf;
    for(auto i = ifc.ifc_len / sizeof(struct ifreq); i > 0; i--) {
        if(ifr->ifr_flags == AF_INET) {
            if (strcmp(ifr->ifr_name, "lo") != 0)
            {
                return inet_ntoa(((struct sockaddr_in*)&(ifr->ifr_addr))->sin_addr);
            }
            ifr++;
        }
    }
    return "0.0.0.0";
}


