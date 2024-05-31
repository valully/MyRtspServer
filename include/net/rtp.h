//
// Created by tcy on 2024/5/19.
//

#ifndef RTSP_RTP_H
#define RTSP_RTP_H

#include <cstdint>
#include <cstdlib>

#define RTP_VESION              2

#define RTP_PAYLOAD_TYPE_H264   96
#define RTP_PAYLOAD_TYPE_AAC    97

#define RTP_HEADER_SIZE         12
#define RTP_MAX_PKT_SIZE        1400

struct RtpHeader {
    /* byte 0 */
    uint8_t csrc_len_:4;
    uint8_t extension_:1;
    uint8_t padding_:1;
    uint8_t version_:2;

    /* byte 1 */
    uint8_t payload_type_:7;
    uint8_t marker_:1;

    /* bytes 2,3 */
    uint16_t seq_;

    /* bytes 4-7 */
    uint32_t timestamp_;

    /* bytes 8-11 */
    uint32_t ssrc_;

    /* data */
    uint8_t payload[0];
};

class RtpPacket {
public:
    RtpPacket() : _m_buffer_(new uint8_t[RTP_HEADER_SIZE + RTP_MAX_PKT_SIZE + 100]),
        m_buffer_(_m_buffer_ + 4), m_rtp_header_((RtpHeader*)m_buffer_), m_size_(0) {}

    ~RtpPacket() { delete[] _m_buffer_; }

    uint8_t* _m_buffer_;
    uint8_t* m_buffer_;
    RtpHeader* const m_rtp_header_;
    int32_t m_size_;
};

#endif //RTSP_RTP_H
