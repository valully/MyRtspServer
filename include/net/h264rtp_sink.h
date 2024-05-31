//
// Created by tcy on 2024/5/24.
//

#ifndef RTSP_H264RTP_SINK_H
#define RTSP_H264RTP_SINK_H

#include <cstring>
#include <cstdio>

#include "rtpsink.h"

class H264RtpSink : public RtpSink {
public:
    static H264RtpSink* CreateNew(UsageEnvironment* env, MediaSource* media_source) {
        if(media_source == nullptr) {
            return nullptr;
        }
        return new H264RtpSink(env, media_source);
    }

    H264RtpSink(UsageEnvironment* env, MediaSource* media_source);
    virtual ~H264RtpSink() = default;

    virtual std::string GetMediaDescription(uint16_t port) override;
    virtual std::string GetAttribute() override;
    virtual void HandleFrame(AVFrame *frame) override;

private:
    RtpPacket m_rtp_packet_;
    int m_clock_rate_;
    int32_t m_fps_;
};


#endif //RTSP_H264RTP_SINK_H
