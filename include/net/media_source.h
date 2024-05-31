//
// Created by tcy on 2024/5/20.
//

#ifndef RTSP_MEDIA_SOURCE_H
#define RTSP_MEDIA_SOURCE_H

#include <queue>
#include <stdint.h>

#include "usageenvironment.h"

#define FRAME_MAX_SIZE (1920*1200)
#define DEFAULT_FRAME_NUM 4

class AVFrame {
public:
    AVFrame(): m_buffer_(new uint8_t[FRAME_MAX_SIZE]) {}
    ~AVFrame() { delete[] m_buffer_; }

    uint8_t * m_buffer_;
    uint8_t * m_frame_;
    int32_t m_frame_size_{0};
};

class MediaSource {
public:
    virtual ~MediaSource() = default;

    AVFrame* GetFrame() {
        std::lock_guard<std::mutex> locker(m_mtx_);
        if(m_av_frame_output_que_.empty()) {
            return nullptr;
        }
        auto frame = m_av_frame_output_que_.front();
        m_av_frame_output_que_.pop();
        return frame;
    }

    void PutFrame(AVFrame* frame) {
        std::lock_guard<std::mutex> locker(m_mtx_);
        m_av_frame_input_que_.push(frame);
        m_env_->ThPool()->AddTask(m_task_);
    }

    int32_t GetFps() const { return m_fps_;}

protected:
    MediaSource(UsageEnvironment* env) : m_env_(env) {
        for(uint32_t i = 0; i < DEFAULT_FRAME_NUM; i++) {
            m_av_frame_input_que_.push(&m_av_frames_[i]);
        }
        m_task_.SetTaskCallback(TaskCallback, this);
    }

    virtual void ReadFrame() = 0;
    void SetFps(int32_t fps) { m_fps_ = fps; }

private:
    static void TaskCallback(void* arg) {
        auto source = (MediaSource*)arg;
        source->ReadFrame();
    }

protected:
    UsageEnvironment* m_env_;
    AVFrame m_av_frames_[DEFAULT_FRAME_NUM];
    std::queue<AVFrame*> m_av_frame_input_que_;
    std::queue<AVFrame*> m_av_frame_output_que_;
    std::mutex m_mtx_;
    ThreadPool::Task m_task_;
    int32_t m_fps_;
};

#endif //RTSP_MEDIA_SOURCE_H
