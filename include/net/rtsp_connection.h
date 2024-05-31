//
// Created by tcy on 2024/5/21.
//

#ifndef RTSP_RTSP_CONNECTION_H
#define RTSP_RTSP_CONNECTION_H

#include <map>
#include "rtp_instance.h"
#include "tcp_connection.h"
#include "rtsp_server.h"
#include "media_session.h"

class RtspServer;

class RtspConnection : public TcpConnection {
public:
    enum Method {
        OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN,
        GET_PARAMETER, RTCP, PAUSE, NONE,
    };

    static RtspConnection* CreateNew(RtspServer* rtsp_server, int32_t sockfd);

    RtspConnection(RtspServer* rtsp_server, int32_t sockfd);
    ~RtspConnection();

protected:
    virtual void HandleReadBytes();

private:
    bool ParseRequest();
    bool ParseRequest1(const char* begin, const char* end);
    bool ParseRequest2(const char* begin, const char* end);

    bool ParseCSeq(const std::string& message);
    bool ParseAccept(const std::string& message);
    bool ParseTransport(const std::string& message);
    bool ParseMediaTrack();
    bool ParseSessionId(const std::string& message);

    bool HandleCmdOption();
    bool HandleCmdDescribe();
    bool HandleCmdSetup();
    bool HandleCmdPlay();
    bool HandleCmdTeardown();
    bool HandleCmdGetParamter();
    bool HandleCmdPause();

    int32_t SendMessage(void* buf, int32_t size);

    bool CreateRtpRtcpOverUdp(MediaSession::TrackId track_id, std::string peer_ip,
                              uint16_t peer_rtp_port, uint16_t peer_rtcp_port);
    bool CreateRtpOverTcp(MediaSession::TrackId track_id, int sockfd, uint8_t rtp_channel);

    void HandleRtpOverTcp();

private:
    RtspServer* m_rtsp_server_;
    std::string m_peer_ip_;
    Method m_method_;
    std::string m_url_;
    std::string m_suffix_;
    uint32_t m_cseq_;
    uint16_t m_peer_rtp_port_;
    uint16_t m_peer_rtcp_port_;
    MediaSession::TrackId m_track_id_;
    RtpInstance* m_rtp_instances_[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* m_rtcp_instances_[MEDIA_MAX_TRACK_NUM];
    MediaSession* m_session_;
    int32_t m_session_id_;
    bool m_is_rtp_over_tcp_{false};
    uint8_t m_rtp_channel_;
};


#endif //RTSP_RTSP_CONNECTION_H
