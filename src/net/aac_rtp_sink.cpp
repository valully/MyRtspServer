//
// Created by tcy on 2024/5/27.
//

#include "../../include/net/aac_rtp_sink.h"

static uint32_t AACSampleRate[16] = {
    97000, 88200, 64000, 48000,
    44100, 32000, 24000, 22050,
    16000, 12000, 11025, 8000,
    7350, 0, 0, 0
};

AACRtpSink* AACRtpSink::CreateNew(UsageEnvironment *env, MediaSource *media_source) {
    return new AACRtpSink(env, media_source, RTP_PAYLOAD_TYPE_AAC);
}

AACRtpSink::AACRtpSink(UsageEnvironment *env, MediaSource *media_source, int32_t payload) :
    RtpSink(env, media_source, payload), m_sample_rate_(44100), m_channels_(2), m_fps_(media_source->GetFps()) {
    m_marker_ = 1;
    Start(1000 / m_fps_);
}

std::string AACRtpSink::GetMediaDescription(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=audio %hu RTP/AVP %d", port, m_payload_type_);
    return buf;
}

std::string AACRtpSink::GetAttribute() {
    char buf[500] = {0};
    sprintf(buf, "a=rtpmap:97 mpeg4-generic/%u/%u\r\n", m_sample_rate_, m_channels_);

    uint8_t index = 0;
    for(; index < 16; index++) {
        if(AACSampleRate[index] == m_sample_rate_) {
            break;
        }
    }
    if(index == 16) {
        return "";
    }

    uint8_t profile = 1;
    char config_str[10] = {0};
    sprintf(config_str, "%02x%02x", (uint8_t)((profile+1) << 3)|(index >> 1),
            (uint8_t)((index << 7)|(m_channels_ << 3)));

    sprintf(buf + strlen(buf),
            "a=fmtp:%d profile-level-id=1;"
            "mode=AAC-hbr;"
            "sizelength=13;indexlength=3;indexdeltalength=3;"
            "config=%04u",
            m_payload_type_, atoi(config_str));
    return buf;
}

void AACRtpSink::HandleFrame(AVFrame *frame) {
    auto rtp_header = m_rtp_packet_.m_rtp_header_;
    auto frame_size = frame->m_frame_size_ - 7;

    rtp_header->payload[0] = 0x00;
    rtp_header->payload[1] = 0x10;
    rtp_header->payload[2] = (frame_size & 0x1FE0) >> 5;
    rtp_header->payload[3] = (frame_size & 0x1F) << 3;

    memcpy(rtp_header->payload + 4, frame->m_frame_ + 7, frame_size);
    m_rtp_packet_.m_size_ = frame_size + 4;

    SendRtpPacket(&m_rtp_packet_);
    m_seq_++;
    m_time_stamp_ += m_sample_rate_ * (1000 / m_fps_) / 1000;
}