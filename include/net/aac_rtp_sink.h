//
// Created by tcy on 2024/5/27.
//

#ifndef RTSP_AAC_RTP_SINK_H
#define RTSP_AAC_RTP_SINK_H

#include "rtpsink.h"

class AACRtpSink : public RtpSink {
public:
    static AACRtpSink* CreateNew(UsageEnvironment* env, MediaSource* media_source);

    AACRtpSink(UsageEnvironment* env, MediaSource* media_source, int32_t payload);

    virtual ~AACRtpSink() = default;

    virtual std::string GetMediaDescription(uint16_t port) override;

    virtual std::string GetAttribute() override;

protected:
    virtual void HandleFrame(AVFrame* frame) override;

private:
    RtpPacket m_rtp_packet_;
    uint32_t m_sample_rate_;
    uint32_t m_channels_;
    int32_t m_fps_;
};


#endif //RTSP_AAC_RTP_SINK_H
