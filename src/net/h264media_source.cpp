//
// Created by tcy on 2024/5/23.
//

#include <fcntl.h>
#include <cassert>

#include "../../include/net/h264media_source.h"

static inline bool StartCode3(const uint8_t* buf) {
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1) {
        return true;
    }
    return false;
}
static inline bool StartCode4(const uint8_t* buf) {
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1) {
        return true;
    }
    return false;
}

H264MediaSource::H264MediaSource(UsageEnvironment *env, const std::string &file) :
        MediaSource(env), m_file_(file) {
    m_fd_ = ::open(file.c_str(), O_RDONLY);
    assert(m_fd_ > 0);

    SetFps(25);
    for(uint32_t i = 0; i < DEFAULT_FRAME_NUM; i++) {
        m_env_->ThPool()->AddTask(m_task_);
    }
}

void H264MediaSource::ReadFrame() {
    std::lock_guard<std::mutex> locker(m_mtx_);

    if(m_av_frame_input_que_.empty()) {
        return;
    }

    auto frame = m_av_frame_input_que_.front();
    frame->m_frame_size_ = GetFrameFromH264File(m_fd_, frame->m_buffer_, FRAME_MAX_SIZE);
    if(frame->m_frame_size_ < 0) {
        return;
    }

    if(StartCode3(frame->m_buffer_)) {
        frame->m_frame_ = frame->m_buffer_ + 3;
        frame->m_frame_size_ -= 3;
    } else {
        frame->m_frame_ = frame->m_buffer_ + 4;
        frame->m_frame_size_ -= 4;
    }

    m_av_frame_input_que_.pop();
    m_av_frame_output_que_.push(frame);
}

static uint8_t * FindNextStartCode(uint8_t* buf, uint32_t len) {
    if(len < 3) {
        return nullptr;
    }
    for(uint32_t i = 0; i < len - 3; i++) {
        if(StartCode3(buf) || StartCode4(buf)) {
            return buf;
        }
        buf++;
    }
    if(StartCode3(buf)) {
        return buf;
    }
    return nullptr;
}

int32_t H264MediaSource::GetFrameFromH264File(int32_t fd, uint8_t *frame, int32_t size) {
    if(fd < 0) {
        return fd;
    }
    int32_t r_size = read(fd, frame, size);
    if(!StartCode3(frame) && !StartCode4(frame)) {
        LOG_WARNING("failed to parse h264 start code\n");
        return -1;
    }

    auto next_start_code = FindNextStartCode(frame + 3, r_size - 3);
    int32_t frame_size;
    if(next_start_code == nullptr) {
        lseek(fd, 0, SEEK_SET);
        frame_size = r_size;
    } else {
        frame_size = next_start_code - frame;
        lseek(fd, frame_size - r_size, SEEK_CUR);
    }
    return frame_size;
}