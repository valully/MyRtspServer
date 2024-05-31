//
// Created by tcy on 2024/5/27.
//

#include <fcntl.h>
#include <cassert>

#include "../../include/net/aac_media_source.h"

AACMediaSource* AACMediaSource::CreateNew(UsageEnvironment *env, const std::string &file) {
    return new AACMediaSource(env, file);
}

AACMediaSource::AACMediaSource(UsageEnvironment *env, const std::string &file) : MediaSource(env), m_file_(file) {
    m_fd_ = ::open(file.c_str(), O_RDONLY);
    assert(m_fd_ > 0);
    SetFps(43);
    for(int i = 0; i < DEFAULT_FRAME_NUM; i++) {
        m_env_->ThPool()->AddTask(m_task_);
    }
}

AACMediaSource::~AACMediaSource() {
    ::close(m_fd_);
}

void AACMediaSource::ReadFrame() {
    std::lock_guard<std::mutex> locker(m_mtx_);

    if(m_av_frame_input_que_.empty()) {
        return;
    }

    auto frame = m_av_frame_input_que_.front();
    frame->m_frame_size_ = GetFrameFromAACFile(m_fd_, frame->m_buffer_, FRAME_MAX_SIZE);
    if(frame->m_frame_size_ < 0) {
        return;
    }

    frame->m_frame_ = frame->m_buffer_;
    m_av_frame_input_que_.pop();
    m_av_frame_output_que_.push(frame);
}

bool AACMediaSource::ParseAdtsHeader(uint8_t *in, AACMediaSource::AdtsHeader *res) {
    memset(res, 0, sizeof(*res));
    if ((in[0] == 0xFF) && ((in[1] & 0xF0) == 0xF0))
    {
        res->id_ = ((uint32_t) in[1] & 0x08) >> 3;
        res->layer_ = ((uint32_t) in[1] & 0x06) >> 1;
        res->protection_absent_ = (uint32_t) in[1] & 0x01;
        res->profile_ = ((uint32_t) in[2] & 0xc0) >> 6;
        res->sampling_freq_index_ = ((uint32_t ) in[2] & 0x3c) >> 2;
        res->private_bit_ = ((uint32_t) in[2] & 0x02) >> 1;
        res->channel_cfg_ = ((((uint32_t) in[2] & 0x01) << 2) | (((uint32_t) in[3] & 0xc0) >> 6));
        res->original_copy_ = ((uint32_t) in[3] & 0x20) >> 5;
        res->home_ = ((uint32_t) in[3] & 0x10) >> 4;
        res->copyright_identification_bit_ = ((uint32_t) in[3] & 0x08) >> 3;
        res->copyright_identification_start_ = (uint32_t) in[3] & 0x04 >> 2;
        res->aac_frame_length_ = (((((uint32_t) in[3]) & 0x03) << 11) |
                               (((uint32_t)in[4] & 0xFF) << 3) |
                               ((uint32_t)in[5] & 0xE0) >> 5) ;
        res->adts_buffer_fullness_ = (((uint32_t) in[5] & 0x1f) << 6 | ((uint32_t) in[6] & 0xfc) >> 2);
        res->number_of_raw_data_block_in_frame_ = ((uint32_t) in[6] & 0x03);
        return true;
    }

    LOG_WARNING("failed to parse adts header\n");
    return false;
}

int32_t AACMediaSource::GetFrameFromAACFile(int32_t fd, uint8_t *buf, uint32_t size) {
    uint8_t tmp_buf[7];
    auto ret = read(fd, tmp_buf, 7);
    if(ret <= 0) {
        lseek(fd, 0, SEEK_SET);
        ret = read(fd, tmp_buf, 7);
        if(ret <= 0) {
            return -1;
        }
    }

    if(!ParseAdtsHeader(tmp_buf, &m_adts_header_)) {
        LOG_WARNING("parse aac err\n");
        return -1;
    }

    if(m_adts_header_.aac_frame_length_ > size) {
        LOG_WARNING("aac_frame_length_ > size\n");
        return -1;
    }

    memcpy(buf, tmp_buf, 7);
    ret = read(fd, buf + 7, m_adts_header_.aac_frame_length_ - 7);
    if(ret < 0) {
        LOG_WARNING("read aac err\n");
        return -1;
    }

    return m_adts_header_.aac_frame_length_;
}
