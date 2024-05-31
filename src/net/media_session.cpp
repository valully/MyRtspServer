//
// Created by tcy on 2024/5/20.
//

#include "../../include/net/media_session.h"

MediaSession::MediaSession(const std::string &session_name) : m_session_name_(session_name){
    m_tracks_[0].m_track_id_ = TrackId::TrackId0;
    m_tracks_[1].m_track_id_ = TrackId::TrackId1;
    m_tracks_[0].m_is_alive_ = false;
    m_tracks_[1].m_is_alive_ = false;

    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        m_multicast_rtp_instances_[i] = nullptr;
        m_multicast_rtcp_instances_[i] = nullptr;
    }
}

MediaSession::~MediaSession() {
    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(m_multicast_rtp_instances_[i]) {
            this->RemoveRtpInstance(m_multicast_rtp_instances_[i]);
            delete m_multicast_rtp_instances_[i];
        }
        if(m_multicast_rtcp_instances_[i]) {
            delete m_multicast_rtcp_instances_[i];
        }
    }
}

std::string MediaSession::GenerateSdpDescription() {
    if(!m_sdp_.empty()) {
        return m_sdp_;
    }
    auto ip = Sockets::GetLocalIp();
    char buf[2048] = {0};
    snprintf(buf, sizeof(buf),
             "v=0\r\n"
             "o=- 9%ld 1 IN IP4 %s\r\n"
             "t=0 0\r\n"
             "a=control:*\r\n"
             "a=type:broadcast\r\n",
             (long)time(nullptr), ip.c_str());

    if(IsStartMulticast()) {
        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "a=rtcp-unicast: reflection\r\n");
    }

    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(!m_tracks_[i].m_is_alive_) {
            continue;
        }

        uint16_t port = 0;
        if(IsStartMulticast()) {
            port = GetMulticastDestRtpPort((TrackId)i);
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "%s\r\n", m_tracks_[i].m_rtp_sink_->GetMediaDescription(port).c_str());

        if(IsStartMulticast()) {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "c=IN IP4 %s/255\r\n", GetMulticastDestAddr().c_str());
        } else {
            snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                     "c=IN IP4 0.0.0.0\r\n");
        }

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "%s\r\n", m_tracks_[i].m_rtp_sink_->GetAttribute().c_str());

        snprintf(buf + strlen(buf), sizeof(buf) - strlen(buf),
                 "a=control:track%d\r\n", m_tracks_[i].m_track_id_);
    }
    m_sdp_ = buf;
    return m_sdp_;
}

bool MediaSession::AddRtpSink(MediaSession::TrackId track_id, RtpSink *rtp_sink) {
    auto track = GetTrack(track_id);
    if(!track) {
        return false;
    }
    track->m_rtp_sink_ = rtp_sink;
    track->m_is_alive_ = true;
    rtp_sink->SetSendFrameCallback(SendPacketCallback, this, track);
    return true;
}

bool MediaSession::AddRtpInstance(MediaSession::TrackId track_id, RtpInstance *rtp_instance) {
    auto track = GetTrack(track_id);
    if(!track || !track->m_is_alive_) {
        return false;
    }
    track->m_rtp_instances_.push_back(rtp_instance);
    return true;
}

bool MediaSession::RemoveRtpInstance(RtpInstance *rtp_instance) {
    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(!m_tracks_[i].m_is_alive_) {
            continue;
        }

        auto iter = std::find(m_tracks_[i].m_rtp_instances_.begin(), m_tracks_[i].m_rtp_instances_.end(), rtp_instance);
        if(iter == m_tracks_[i].m_rtp_instances_.end()) {
            continue;
        }
        m_tracks_[i].m_rtp_instances_.erase(iter);
        return true;
    }
    return false;
}

void MediaSession::SendPacketCallback(void *arg1, void *arg2, RtpPacket *rtp_packet) {
    auto media_session = (MediaSession*)arg1;
    auto track = (MediaSession::Track*)arg2;
    media_session->SendPacket(track, rtp_packet);
}

void MediaSession::SendPacket(MediaSession::Track *track, RtpPacket *rtp_packet) {
    for(auto&iter:track->m_rtp_instances_) {
        if(iter->Alive()) {
            iter->Send(rtp_packet);
        }
    }
}

bool MediaSession::StartMulticast() {
    struct sockaddr_in addr = {0};
    uint32_t range = 0xE8FFFFFF - 0xE8000100;
    addr.sin_addr.s_addr = htonl(0xE8000100 + (rand()) % range);
    m_multicast_addr_ = inet_ntoa(addr.sin_addr);

    auto rtp_sockfd1 = Sockets::CreateUdpSock();
    assert(rtp_sockfd1 > 0);

    auto rtp_sockfd2 = Sockets::CreateUdpSock();
    assert(rtp_sockfd2 > 0);

    auto rtcp_sockfd1 = Sockets::CreateUdpSock();
    assert(rtcp_sockfd1 > 0);

    auto rtcp_sockfd2 = Sockets::CreateUdpSock();
    assert(rtcp_sockfd2 > 0);

    uint16_t port = rand() & 0xfffe;
    if(port < 10000)
        port += 10000;

    uint16_t rtp_port1 = port;
    uint16_t rtcp_port1 = port + 1;
    uint16_t rtp_port2 = rtcp_port1 + 1;
    uint16_t rtcp_port2 = rtp_port2 + 1;

    m_multicast_rtp_instances_[TrackId0] = RtpInstance::CreateNewOverUdp(rtp_sockfd1, 0, m_multicast_addr_, rtp_port1);
    m_multicast_rtp_instances_[TrackId1] = RtpInstance::CreateNewOverUdp(rtp_sockfd2, 0, m_multicast_addr_, rtp_port2);
    m_multicast_rtcp_instances_[TrackId0] = RtcpInstance::CreateNew(rtcp_sockfd1, 0, m_multicast_addr_, rtcp_port1);
    m_multicast_rtcp_instances_[TrackId1] = RtcpInstance::CreateNew(rtcp_sockfd2, 0, m_multicast_addr_, rtcp_port2);

    this->AddRtpInstance(TrackId0, m_multicast_rtp_instances_[TrackId0]);
    this->AddRtpInstance(TrackId1, m_multicast_rtp_instances_[TrackId1]);
    m_is_start_multicast_ = true;
    return true;
}


