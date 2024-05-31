//
// Created by tcy on 2024/5/24.
//

#include "../../include/net/h264rtp_sink.h"

H264RtpSink::H264RtpSink(UsageEnvironment *env, MediaSource *media_source) :
    RtpSink(env, media_source, RTP_PAYLOAD_TYPE_H264), m_clock_rate_(90000), m_fps_(media_source->GetFps()) {
    Start(1000 / m_fps_);
}

std::string H264RtpSink::GetMediaDescription(uint16_t port) {
    char buf[100] = {0};
    sprintf(buf, "m=video %hu RTP/AVP %d", port, m_payload_type_);
    return buf;
}

std::string H264RtpSink::GetAttribute() {
    char buf[100] = {0};
    sprintf(buf, "a=rtpmap:%d H264/%d\r\n", m_payload_type_, m_clock_rate_);
    sprintf(buf + strlen(buf), "a=framerate:%d", m_fps_);
    return buf;
}

void H264RtpSink::HandleFrame(AVFrame *frame) {
    auto rtp_header = m_rtp_packet_.m_rtp_header_;
    uint8_t nalu_type = frame->m_frame_[0];
    if(frame->m_frame_size_ <= RTP_MAX_PKT_SIZE) {
        memcpy(rtp_header->payload, frame->m_frame_, frame->m_frame_size_);
        m_rtp_packet_.m_size_ = frame->m_frame_size_;
        SendRtpPacket(&m_rtp_packet_);
        m_seq_++;
        if((nalu_type & 0x1F) == 7 || (nalu_type & 0x1F) == 8) {
            return;
        }
    } else {
        int pkt_num = frame->m_frame_size_ / RTP_MAX_PKT_SIZE;
        int remain_pkt_size = frame->m_frame_size_ % RTP_MAX_PKT_SIZE;
        int pos = 1;
        for(int i = 0; i < pkt_num; i++) {
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;
            rtp_header->payload[1] = nalu_type & 0x1f;
            if(i == 0) {
                rtp_header->payload[1] |= 0x80;
            } else if(remain_pkt_size == 0 && i == pkt_num - 1) {
                rtp_header->payload[1] |= 0x40;
            }
            memcpy(rtp_header->payload + 2, frame->m_frame_ + pos, RTP_MAX_PKT_SIZE);
            m_rtp_packet_.m_size_ = RTP_MAX_PKT_SIZE + 2;
            SendRtpPacket(&m_rtp_packet_);
            m_seq_++;
            pos += RTP_MAX_PKT_SIZE;
        }

        if(remain_pkt_size > 0) {
            rtp_header->payload[0] = (nalu_type & 0x60) | 28;
            rtp_header->payload[1] = nalu_type & 0x1f;
            rtp_header->payload[1] = 0x40;

            memcpy(rtp_header->payload + 2, frame->m_frame_ + pos, remain_pkt_size);
            m_rtp_packet_.m_size_ = remain_pkt_size + 2;
            SendRtpPacket(&m_rtp_packet_);
            m_seq_++;
        }
    }
    m_time_stamp_ += m_clock_rate_ / m_fps_;
}