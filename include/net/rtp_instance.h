//
// Created by tcy on 2024/5/19.
//

#ifndef RTSP_RTP_INSTANCE_H
#define RTSP_RTP_INSTANCE_H

#include "inet_address.h"
#include "socket_ops.h"
#include "rtp.h"

class RtpInstance {
public:
    enum RtpType {
        RTP_OVER_UDP,
        RTP_OVER_TCP
    };

    static RtpInstance* CreateNewOverUdp(int32_t local_sockfd, uint16_t local_port,
                                         const std::string& dest_ip, uint16_t dest_port) {
        return new RtpInstance(local_sockfd, local_port, dest_ip, dest_port);
    }

    static RtpInstance* CreateNewOverTcp(int32_t client_fd, uint8_t rtp_channel) {
        return new RtpInstance(client_fd, rtp_channel);
    }

    RtpInstance(int32_t local_sockfd, uint16_t local_port, const std::string& dest_ip, uint16_t dest_port) :
        m_rtp_type_(RTP_OVER_UDP), m_sock_fd_(local_sockfd), m_local_port_(local_port), m_dest_addr_(dest_ip, dest_port) {}

    RtpInstance(int32_t client_sockfd, uint8_t rtp_channel) :
        m_rtp_type_(RTP_OVER_TCP), m_sock_fd_(client_sockfd), m_rtp_channel_(rtp_channel) {}

    ~RtpInstance() { Sockets::Close(m_sock_fd_); }

    uint16_t GetLocalPort() { return m_local_port_; }
    uint16_t GetPeerPort() { return m_dest_addr_.GetPort(); }

    bool Alive() const { return m_is_alive_; }
    void SetAlive(bool alive) { m_is_alive_ = alive; }
    void SetSessionId(uint16_t session_id) { m_session_id_ = session_id; }
    uint16_t SessionId() const { return m_session_id_; }

    int32_t Send(RtpPacket* rtp_packet) {
        if(m_rtp_type_ == RTP_OVER_UDP) {
            return SendOverUdp(rtp_packet->m_buffer_, rtp_packet->m_size_);
        }
        uint8_t * rtp_pkt_ptr = rtp_packet->_m_buffer_;
        rtp_pkt_ptr[0] = '$';
        rtp_pkt_ptr[1] = (uint8_t)m_rtp_channel_;
        rtp_pkt_ptr[2] = (uint8_t)((rtp_packet->m_size_ & 0xFF00) >> 8);
        rtp_pkt_ptr[3] = (uint8_t)(rtp_packet->m_size_ & 0xFF);
        return SendOverTcp(rtp_pkt_ptr, rtp_packet->m_size_ + 4);
    }

private:
    int32_t SendOverUdp(void* buf, int32_t size) {
        return Sockets::Sendto(m_sock_fd_, buf, size, m_dest_addr_.GetAddr());
    }

    int32_t SendOverTcp(void* buf, int32_t size) {
        return Sockets::Write(m_sock_fd_, buf, size);
    }

private:
    RtpType m_rtp_type_;
    int32_t m_sock_fd_;
    uint16_t m_local_port_;
    Ipv4Address m_dest_addr_;
    bool m_is_alive_{false};
    uint16_t m_session_id_{0};
    uint16_t m_rtp_channel_;
};

class RtcpInstance {
public:
    static RtcpInstance* CreateNew(int32_t local_sockfd, uint16_t local_port, const std::string& dest_ip, uint16_t dest_port) {
        return new RtcpInstance(local_sockfd, local_port, dest_ip, dest_port);
    }

    RtcpInstance(int32_t local_sockfd, uint16_t local_port, const std::string& dest_ip, uint16_t dest_port) :
        m_local_sockfd_(local_sockfd), m_local_port_(local_port), m_dest_addr_(dest_ip, dest_port) {}

    ~RtcpInstance() { Sockets::Close(m_local_sockfd_); }

    uint16_t GetLocalPort() const { return m_local_port_; }

    bool Alive() const { return m_is_alive_; }
    void SetAlive(bool alive) { m_is_alive_ = alive; }
    void SetSessionId(uint16_t session_id) { m_session_id_ = session_id; }
    uint16_t SessionId() const { return m_session_id_; }

    int32_t Send(void* buf, int32_t size) {
        return Sockets::Sendto(m_local_sockfd_, buf, size, m_dest_addr_.GetAddr());
    }

    int32_t Recv(void* buf, int size, Ipv4Address* addr) {
        return 0;
    }

private:
    int32_t m_local_sockfd_;
    uint16_t m_local_port_;
    Ipv4Address m_dest_addr_;
    bool m_is_alive_;
    uint16_t m_session_id_;
};

#endif //RTSP_RTP_INSTANCE_H
