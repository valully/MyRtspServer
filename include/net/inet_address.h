//
// Created by tcy on 2024/5/18.
//

#ifndef RTSP_INET_ADDRESS_H
#define RTSP_INET_ADDRESS_H

#include <string>
#include <stdint.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>

class Ipv4Address {
public:
    Ipv4Address() = default;
    Ipv4Address(const std::string& ip, uint16_t port) : m_ip_(ip), m_port_(port) {
        m_addr_.sin_family = AF_INET;
        m_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
        m_addr_.sin_port = htons(m_port_);
    }

    void SetAddr(const std::string& ip, uint16_t port) {
        m_ip_ = ip;
        m_port_ = port;
        m_addr_.sin_family = AF_INET;
        m_addr_.sin_addr.s_addr = inet_addr(ip.c_str());
        m_addr_.sin_port = htons(port);
    }
    std::string GetIp() { return m_ip_; }
    uint16_t GetPort() { return m_port_; }
    struct sockaddr* GetAddr() { return (struct sockaddr*)&m_addr_; }

private:
    std::string m_ip_;
    uint16_t m_port_;
    struct sockaddr_in m_addr_;
};

#endif //RTSP_INET_ADDRESS_H
