//
// Created by tcy on 2024/5/20.
//

#ifndef RTSP_RTPSINK_H
#define RTSP_RTPSINK_H

#include <arpa/inet.h>

#include "media_source.h"
#include "event.h"
#include "rtp.h"

class RtpSink {
public:
    typedef void(*SendPacketCallback)(void* arg1, void* arg2, RtpPacket* media_packet);

    RtpSink(UsageEnvironment* env, MediaSource* media_source, uint8_t payload_type) :
        m_env_(env), m_media_source_(media_source), m_payload_type_(payload_type) {
        m_timer_event_ = TimerEvent::CreateNew(this);
        m_timer_event_->SetCallBack(TimeoutCallback);
        m_ssrc_ = rand();
    }

    virtual ~RtpSink() {
        m_env_->Scheduler()->RemoveTimerEvent(m_timer_id_);
        delete m_timer_event_;
    }

    virtual std::string GetMediaDescription(uint16_t port) = 0;
    virtual std::string GetAttribute() = 0;

    void SetSendFrameCallback(SendPacketCallback cb, void* arg1, void * arg2) {
        m_send_packet_cb_ = cb;
        m_arg1_ = arg1;
        m_arg2_ = arg2;
    }

protected:
    virtual void HandleFrame(AVFrame* frame) = 0;

    void SendRtpPacket(RtpPacket* packet) {
        auto rtp_head = packet->m_rtp_header_;
        rtp_head->csrc_len_ = m_csrc_len_;
        rtp_head->extension_ = m_extension_;
        rtp_head->padding_ = m_padding_;
        rtp_head->version_ = m_version_;
        rtp_head->payload_type_ = m_payload_type_;
        rtp_head->marker_ = m_marker_;
        rtp_head->seq_ = htons(m_seq_);
        rtp_head->timestamp_ = htonl(m_time_stamp_);
        rtp_head->ssrc_ = htonl(m_ssrc_);
        packet->m_size_ += RTP_HEADER_SIZE;

        if(m_send_packet_cb_) {
            m_send_packet_cb_(m_arg1_, m_arg2_, packet);
        }
    }

    void Start(uint32_t ms) {
        m_timer_id_ = m_env_->Scheduler()->AddTimerEventRunEvery(m_timer_event_, ms);
    }

    void Stop() {
        m_env_->Scheduler()->RemoveTimerEvent(m_timer_id_);
    }

private:
    static void TimeoutCallback(void* arg) {
        auto rtp_sink = (RtpSink*)arg;
        auto frame = rtp_sink->m_media_source_->GetFrame();
        if(!frame) {
            return;
        }
        rtp_sink->HandleFrame(frame);
        rtp_sink->m_media_source_->PutFrame(frame);
    }

protected:
    UsageEnvironment* m_env_;
    MediaSource* m_media_source_;
    SendPacketCallback m_send_packet_cb_{nullptr};
    void* m_arg1_{nullptr};
    void* m_arg2_{nullptr};

    uint8_t m_csrc_len_{0};
    uint8_t m_extension_{0};
    uint8_t m_padding_{0};
    uint8_t m_version_{RTP_VESION};
    uint8_t m_payload_type_;
    uint8_t m_marker_{0};
    uint16_t m_seq_{0};
    uint32_t m_time_stamp_{0};
    uint32_t m_ssrc_;

private:
    TimerEvent* m_timer_event_;
    Timer::TimerId m_timer_id_;
};

#endif //RTSP_RTPSINK_H
