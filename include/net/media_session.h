//
// Created by tcy on 2024/5/20.
//

#ifndef RTSP_MEDIA_SESSION_H
#define RTSP_MEDIA_SESSION_H

#include <string>
#include <list>
#include <algorithm>
#include <cassert>
#include "rtpsink.h"
#include "rtp_instance.h"

#define MEDIA_MAX_TRACK_NUM 2

class MediaSession {
public:
    enum TrackId {
        TrackIdNone = -1,
        TrackId0 = 0,
        TrackId1 = 1
    };

    static MediaSession* CreateNew(const std::string& session_name) {
        return new MediaSession(session_name);
    }

    MediaSession(const std::string& session_name);

    ~MediaSession();

    std::string Name() const { return m_session_name_; }

    std::string GenerateSdpDescription();

    bool AddRtpSink(MediaSession::TrackId track_id, RtpSink* rtp_sink);

    bool AddRtpInstance(MediaSession::TrackId track_id, RtpInstance* rtp_instance);

    bool RemoveRtpInstance(RtpInstance* rtp_instance);

    bool StartMulticast();

    bool IsStartMulticast() const {
        return m_is_start_multicast_;
    }

    std::string GetMulticastDestAddr() const { return m_multicast_addr_; }

    uint16_t GetMulticastDestRtpPort(TrackId track_id) {
        if(track_id > TrackId1 || !m_multicast_rtp_instances_[track_id]) {
            return -1;
        }
        return m_multicast_rtp_instances_[track_id]->GetPeerPort();
    }

private:
    class Track {
    public:
        RtpSink* m_rtp_sink_;
        int32_t m_track_id_;
        bool m_is_alive_;
        std::list<RtpInstance*> m_rtp_instances_;
    };

    Track* GetTrack(MediaSession::TrackId track_id) {
        for(uint32_t i = 0; i < MEDIA_MAX_TRACK_NUM; i++) {
            if(m_tracks_[i].m_track_id_ == track_id) {
                return &m_tracks_[i];
            }
        }
        return nullptr;
    }

    static void SendPacketCallback(void* arg1, void* arg2, RtpPacket* rtp_packet);

    void SendPacket(MediaSession::Track* track, RtpPacket* rtp_packet);

private:
    std::string m_session_name_;
    std::string m_sdp_;
    Track m_tracks_[MEDIA_MAX_TRACK_NUM];
    bool m_is_start_multicast_{false};
    std::string m_multicast_addr_;
    RtpInstance* m_multicast_rtp_instances_[MEDIA_MAX_TRACK_NUM];
    RtcpInstance* m_multicast_rtcp_instances_[MEDIA_MAX_TRACK_NUM];
};


#endif //RTSP_MEDIA_SESSION_H
