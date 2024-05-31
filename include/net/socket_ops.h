//
// Created by tcy on 2024/5/17.
//

#ifndef RTSP_SOCKET_OPS_H
#define RTSP_SOCKET_OPS_H

#include <string>
#include <sys/uio.h>
#include <arpa/inet.h>

namespace Sockets {
    int32_t CreateTcpSock();
    int32_t CreateUdpSock();
    bool Bind(int32_t sockfd, const std::string& ip, uint16_t port);
    bool Listen(int32_t sockfd, int32_t backlog);
    int32_t Accept(int32_t sockfd);
    int32_t Readv(int32_t sockfd, const struct iovec* iov, int32_t iovcnt);
    int32_t Write(int32_t sockfd, const void* buf, int32_t size);
    int32_t Sendto(int32_t sockfd, const void* buf, int32_t len, const struct sockaddr *dest_addr);
    void SetNonBlock(int32_t sockfd);
    void SetBlock(int32_t sockfd, int32_t write_timeout);
    void SetReuseAddr(int32_t sockfd, int32_t on);
    void SetReusePort(int32_t sockfd);
    void SetNonBlockAndCloseOnExec(int32_t sockfd);
    void IgnoreSigPipeOnSocket(int32_t socketfd);
    void SetNoDelay(int32_t sockfd);
    void SetKeepAlive(int32_t sockfd);
    void SetNoSigpipe(int32_t sockfd);
    void SetSendBufSize(int32_t sockfd, int32_t size);
    void SetRecvBufSize(int32_t sockfd, int32_t size);
    std::string GetPeerIp(int32_t sockfd);
    int16_t GetPeerPort(int32_t sockfd);
    int32_t GetPeerAddr(int32_t sockfd, struct sockaddr_in *addr);
    void Close(int32_t sockfd);
    bool Connect(int32_t sockfd, const std::string& ip, uint16_t port, int32_t timeout);
    std::string GetLocalIp();
}


#endif //RTSP_SOCKET_OPS_H
