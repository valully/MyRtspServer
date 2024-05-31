//
// Created by tcy on 2024/5/21.
//

#include "../../include/net/rtsp_connection.h"

static void GetPeerId(int32_t sockfd, std::string& ip) {
    struct sockaddr_in addr;
    socklen_t addr_len = sizeof(addr);
    getpeername(sockfd, (sockaddr*)&addr, &addr_len);
    ip = inet_ntoa(addr.sin_addr);
}

RtspConnection* RtspConnection::CreateNew(RtspServer *rtsp_server, int32_t sockfd) {
    return new RtspConnection(rtsp_server, sockfd);
}

RtspConnection::RtspConnection(RtspServer *rtsp_server, int32_t sockfd) :
        TcpConnection(rtsp_server->Envir(), sockfd), m_rtsp_server_(rtsp_server),
        m_method_(NONE), m_track_id_(MediaSession::TrackIdNone), m_session_id_(rand()) {
    memset(m_rtp_instances_, 0, sizeof(m_rtp_instances_));
    memset(m_rtcp_instances_, 0, sizeof(m_rtcp_instances_));
    GetPeerId(sockfd, m_peer_ip_);
}

RtspConnection::~RtspConnection() {
    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(m_rtp_instances_[i]) {
            if(m_session_) {
                m_session_->RemoveRtpInstance(m_rtp_instances_[i]);
            }
            delete m_rtp_instances_[i];
        }
        if(m_rtcp_instances_[i]) {
            delete m_rtcp_instances_[i];
        }
    }
}

void RtspConnection::HandleReadBytes() {
    bool ret;
    if(m_is_rtp_over_tcp_) {
        if(m_input_buffer_.Peek()[0] == '$') {
            HandleRtpOverTcp();
            return;
        }
    }

    ret = ParseRequest();
    if(!ret) {
        LOG_ERROR("fail to parse request!\n");
        goto ERR;
    }

    switch (m_method_) {
        case OPTIONS:
            if(!HandleCmdOption()) {
                goto ERR;
            }
            break;

        case DESCRIBE:

            if(!HandleCmdDescribe()) {
                goto ERR;
            }
            break;

        case SETUP:
            if(!HandleCmdSetup()) {
                goto ERR;
            }
            break;

        case PLAY:
            if(!HandleCmdPlay()) {
                goto ERR;
            }
            break;

        case TEARDOWN:
            if(!HandleCmdTeardown()) {
                goto ERR;
            }
            break;

        case GET_PARAMETER:
            if(!HandleCmdGetParamter()) {
                goto ERR;
            }
            break;

        case PAUSE:
            if(!HandleCmdPause()) {
                goto ERR;
            }
            break;

        default:
            goto ERR;
    }

    return;

    ERR:
        HandleDisconnection();
}

bool RtspConnection::ParseRequest() {
    auto crlf = m_input_buffer_.FindCRLF();
    if(crlf == nullptr) {
        LOG_ERROR("parse request err!\n");
        m_input_buffer_.RetrieveAll();
        return false;
    }

    bool ret = ParseRequest1(m_input_buffer_.Peek(), crlf);
    if(!ret) {
        LOG_ERROR("parse request1 err!\n");
        m_input_buffer_.RetrieveAll();
        return false;
    }

    m_input_buffer_.RetrieveUntil(crlf + 2);
    crlf = m_input_buffer_.FindLastCRLF();
    if(crlf == nullptr) {
        LOG_ERROR("find last crlf err!\n");
        m_input_buffer_.RetrieveAll();
        return false;
    }

    ret = ParseRequest2(m_input_buffer_.Peek(), crlf);
    if(!ret) {
        LOG_ERROR("parse request2 err!\n");
        m_input_buffer_.RetrieveAll();
        return false;
    }
    m_input_buffer_.RetrieveUntil(crlf + 2);
    return true;
}

bool RtspConnection::ParseRequest1(const char *begin, const char *end) {
    std::string message(begin, end);
    char method[64] = {0};
    char url[512] = {0};
    char version[64] = {0};

    if(sscanf(message.c_str(), "%s %s %s", method, url, version) != 3) {
        return false;
    }

    if(!strcmp(method, "OPTIONS")) {
        m_method_ = OPTIONS;
    }
    else if(!strcmp(method, "DESCRIBE")) {
        m_method_ = DESCRIBE;
    }
    else if(!strcmp(method, "SETUP")) {
        m_method_ = SETUP;
    }
    else if(!strcmp(method, "PLAY")) {
        m_method_ = PLAY;
    }
    else if(!strcmp(method, "TEARDOWN")) {
        m_method_ = TEARDOWN;
    }
    else if(!strcmp(method, "GET_PARAMETER")) {
        m_method_ = GET_PARAMETER;
    }
    else if(!strcmp(method, "PAUSE")) {
        m_method_ = PAUSE;
    }
    else {
        m_method_ = NONE;
        return false;
    }

    if(strncmp(url, "rtsp://", 7) != 0) {
        return false;
    }

    uint16_t port = 0;
    char ip[64] = {0};
    char suffix[64] = {0};

    if(sscanf(url+7, "%[^:]:%hu/%s", ip, &port, suffix) == 3) {
        m_url_ = url;
        m_suffix_ = suffix;
        return true;
    }
    if(sscanf(url+7, "%[^/]/%s", ip, suffix) == 2) {
        port = 554;
    }
    else {
        return false;
    }

    m_url_ = url;
    m_suffix_ = suffix;
    return true;
}

bool RtspConnection::ParseCSeq(const std::string &message) {
    auto pos = message.find("CSeq");
    if(pos != std::string::npos) {
        if(sscanf(message.c_str() + pos, "%*[^:]: %u", &m_cseq_) != 1) {
            return false;
        }
        return true;
    }
    return false;
}

bool RtspConnection::ParseAccept(const std::string &message) {
    if(message.rfind("Accept") == std::string::npos || message.rfind("sdp") == std::string::npos) {
        return false;
    }
    return true;
}

bool RtspConnection::ParseTransport(const std::string &message) {
    auto pos = message.find("Transport");
    if(pos != std::string::npos) {
        if(pos = message.find("RTP/AVP/TCP"); pos != std::string::npos) {
            m_is_rtp_over_tcp_ = true;
            uint8_t rtcp_channel;
            if(sscanf(message.c_str() + pos, "%*[^;];%*[^=]=%hhu-%hhu",
                      &m_rtp_channel_, &rtcp_channel) != 2) {
                return false;
            }
            return true;
        }
        if(pos = message.find("RTP/AVP"); pos != std::string::npos) {
            if(message.find("unicast", pos) != std::string::npos) {
                if(sscanf(message.c_str() + pos, "%*[^;];%*[^=]=%hu-%hu",
                          &m_peer_rtp_port_, &m_peer_rtcp_port_) != 2) {
                    return false;
                }
                return true;
            }
            if(message.find("multicast", pos) != std::string::npos) {
                return true;
            }
            //return false;
        }
        //return false;
    }
    return false;
}

bool RtspConnection::ParseMediaTrack() {
    auto pos = m_url_.find("track0");
    if(pos != std::string::npos) {
        m_track_id_ = MediaSession::TrackId0;
        return true;
    }

    pos = m_url_.find("track1");
    if(pos != std::string::npos) {
        m_track_id_ = MediaSession::TrackId1;
        return true;
    }
    return false;
}

bool RtspConnection::ParseSessionId(const std::string &message) {
    auto pos = message.find("Session");
    if(pos != std::string::npos) {
        uint32_t session_id;
        if(sscanf(message.c_str() + pos, "%*[^:]: %u", &session_id) != 1) {
            return false;
        }
        return true;
    }
    return false;
}

bool RtspConnection::ParseRequest2(const char *begin, const char *end) {
    std::string message(begin, end);
    if(!ParseCSeq(message)) {
        return false;
    }
    switch (m_method_) {
        case OPTIONS:
            return true;
        case DESCRIBE:
            return ParseAccept(message);
        case SETUP:
            if(!ParseTransport(message)) {
                return false;
            }
            return ParseMediaTrack();
        case PLAY:
            return ParseSessionId(message);
        case TEARDOWN:
        case GET_PARAMETER:
        case PAUSE:
            return true;
        default:
            return false;
    }
}

bool RtspConnection::HandleCmdOption() {
    snprintf(m_buffer_, sizeof(m_buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Public: OPTIONS, DESCRIBE, SETUP, TEARDOWN, PLAY\r\n"
             "\r\n",
             m_cseq_);
    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }
    return true;
}

bool RtspConnection::HandleCmdDescribe() {
    auto session = m_rtsp_server_->LookupMediaSession(m_suffix_);
    if(session == nullptr) {
        LOG_DEBUG("can't look up %s session", m_suffix_.c_str());
        return false;
    }

    m_session_ = session;
    auto sdp = session->GenerateSdpDescription();

    memset(m_buffer_, 0, sizeof(m_buffer_));
    snprintf(m_buffer_, sizeof(m_buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %u\r\n"
             "Content-Length: %u\r\n"
             "Content-Type: application/sdp\r\n"
             "\r\n"
             "%s",
             m_cseq_, (unsigned int)sdp.size(), sdp.c_str());

    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }
    return true;
}

bool RtspConnection::HandleCmdSetup() {
    char session_name[100];
    if(sscanf(m_suffix_.c_str(), "%[^/]/", session_name) != 1) {
        return false;
    }
    auto session = m_rtsp_server_->LookupMediaSession(session_name);
    if(session == nullptr) {
        LOG_DEBUG("can't loop up %s session\n", session_name);
        return false;
    }

    if(m_rtp_instances_[m_track_id_] || m_rtcp_instances_[m_track_id_]) {
        return false;
    }

    if(m_session_->IsStartMulticast()) {
        snprintf(m_buffer_, sizeof(m_buffer_),
                 "RTP/1.0 200 OK\r\n"
                 "CSeq: %d\r\n"
                 "Transport: RTP/AVP;multicast\r\n"
                 "destination=%s;source=%s;port:%d-%d;ttl=255\r\n"
                 "Session: %08x\r\n"
                 "\r\n",
                 m_cseq_,
                 session->GetMulticastDestAddr().c_str(),
                 Sockets::GetLocalIp().c_str(),
                 session->GetMulticastDestRtpPort(m_track_id_),
                 session->GetMulticastDestRtpPort(m_track_id_) + 1,
                 m_session_id_);
    } else{
        if(m_is_rtp_over_tcp_) {
            CreateRtpOverTcp(m_track_id_, m_socket_.GetSockFd(), m_rtp_channel_);
            m_rtp_instances_[m_track_id_]->SetSessionId(m_session_id_);
            session->AddRtpInstance(m_track_id_, m_rtp_instances_[m_track_id_]);
            snprintf(m_buffer_, sizeof(m_buffer_),
                     "RTSP/1.0 200 OK\r\n"
                     "CSeq: %d\r\n"
                     "Transport: RTP/AVP/TCP;unicast;interleaved=%hhu-%hhu\r\n"
                     "Session: %08x\r\n"
                     "\r\n",
                     m_cseq_, m_rtp_channel_, m_rtp_channel_ + 1, m_session_id_);
        } else {
            if(!CreateRtpRtcpOverUdp(m_track_id_, m_peer_ip_, m_peer_rtp_port_, m_peer_rtcp_port_)) {
                LOG_WARNING("fail to create rtp udp!\n");
                return false;
            }
            m_rtp_instances_[m_track_id_]->SetSessionId(m_session_id_);
            m_rtcp_instances_[m_track_id_]->SetSessionId(m_session_id_);

            session->AddRtpInstance(m_track_id_, m_rtp_instances_[m_track_id_]);

            snprintf(m_buffer_, sizeof(m_buffer_),
                     "RTSP/1.0 200 OK\r\n"
                     "CSeq: %u\r\n"
                     "Transport: RTP/AVP;unicast;client_port=%hu-%hu;server_port=%hu-%hu\r\n"
                     "Session: %08x\r\n"
                     "\r\n",
                     m_cseq_,
                     m_peer_rtp_port_,
                     m_peer_rtcp_port_,
                     m_rtp_instances_[m_track_id_]->GetLocalPort(),
                     m_rtcp_instances_[m_track_id_]->GetLocalPort(),
                     m_session_id_);
        }
    }
    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }
    return true;
}

bool RtspConnection::HandleCmdPlay() {
    snprintf(m_buffer_, sizeof(m_buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "Range: npt=0.000-\r\n"
             "Session: %08x; timeout=60\r\n"
             "\r\n",
             m_cseq_,
             m_session_id_);

    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }

    for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
        if(m_rtp_instances_[i]) {
            m_rtp_instances_[i]->SetAlive(true);
        }
        if(m_rtcp_instances_[i]) {
            m_rtcp_instances_[i]->SetAlive(true);
        }
    }

    return true;
}

bool RtspConnection::HandleCmdTeardown() {
    snprintf(m_buffer_, sizeof(m_buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "\r\n",
             m_cseq_);
    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }

    return true;
}

bool RtspConnection::HandleCmdGetParamter() {
    return true;
}

bool RtspConnection::HandleCmdPause() {
    snprintf(m_buffer_, sizeof(m_buffer_),
             "RTSP/1.0 200 OK\r\n"
             "CSeq: %d\r\n"
             "\r\n",
             m_cseq_);
    if(SendMessage(m_buffer_, strlen(m_buffer_)) < 0) {
        return false;
    }
    return true;
}

int32_t RtspConnection::SendMessage(void *buf, int32_t size) {
    m_output_buffer_.Append(buf, size);
    auto ret = m_output_buffer_.Write(m_socket_.GetSockFd());
    m_output_buffer_.RetrieveAll();
    return ret;
}

bool RtspConnection::CreateRtpRtcpOverUdp(MediaSession::TrackId track_id, std::string peer_ip, uint16_t peer_rtp_port,
                                          uint16_t peer_rtcp_port) {
    if(m_rtp_instances_[track_id] || m_rtcp_instances_[track_id]) {
        return false;
    }

    int32_t rtp_sockfd, rtcp_sockfd;
    uint16_t rtp_port, rtcp_port;
    bool ret;

    int32_t i;
    for(i = 0; i < 10; i++) {
        if(rtp_sockfd = Sockets::CreateUdpSock(); rtp_sockfd < 0) {
            return false;
        }
        if(rtcp_sockfd = Sockets::CreateUdpSock(); rtcp_sockfd < 0) {
            Sockets::Close(rtp_sockfd);
            return false;
        }

        uint16_t port = rand() % 0xfffe;
        if(port < 10000) {
            port += 10000;
        }

        rtp_port = port + 1;
        rtcp_port = rtp_port + 1;

        ret = Sockets::Bind(rtp_sockfd, "0.0.0.0", rtp_port);
        if(!ret) {
            Sockets::Close(rtp_sockfd);
            Sockets::Close(rtcp_sockfd);
            continue;
        }

        ret = Sockets::Bind(rtcp_sockfd, "0.0.0.0", rtcp_port);
        if(!ret) {
            Sockets::Close(rtp_sockfd);
            Sockets::Close(rtcp_sockfd);
            continue;
        }
        break;
    }

    if(i == 10) {
        return false;
    }
    m_rtp_instances_[track_id] = RtpInstance::CreateNewOverUdp(rtp_sockfd, rtp_port, peer_ip, peer_rtp_port);
    m_rtcp_instances_[track_id] = RtcpInstance::CreateNew(rtcp_sockfd, rtcp_port, peer_ip, peer_rtcp_port);
    return true;
}

bool RtspConnection::CreateRtpOverTcp(MediaSession::TrackId track_id, int sockfd, uint8_t rtp_channel) {
    m_rtp_instances_[track_id] = RtpInstance::CreateNewOverTcp(sockfd, rtp_channel);
    return true;
}

void RtspConnection::HandleRtpOverTcp() {
    auto buf = (uint8_t*)m_input_buffer_.Peek();
    uint16_t size = (buf[2] << 8) | buf[3];

    if(m_input_buffer_.ReadableBytes() < size + 4) {
        return;
    }
    m_input_buffer_.Retrieve(size + 4);
}

